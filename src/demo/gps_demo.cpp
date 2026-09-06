#include "demo/gps_demo.hpp"

#include "graphics/particle_gpu.hpp"
#include "simulation/work_group_count.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace gps {
namespace {

constexpr std::string_view vertex_shader_source = R"glsl(#version 460 core
struct Particle {
    vec4 positionAge;
    vec4 velocityLifetime;
    uvec4 randomState;
};

layout(std430, binding = 0) readonly buffer ParticleBuffer {
    Particle particles[];
};

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uPointSize;

void main() {
    uint particleIndex = uint(gl_VertexID);
    Particle particle = particles[particleIndex];

    gl_Position = uProjection * uView * uModel * vec4(particle.positionAge.xyz, 1.0);
    gl_PointSize = uPointSize;
}
)glsl";

constexpr std::string_view fragment_shader_source = R"glsl(#version 460 core
layout(location = 0) out vec4 fragmentColor;

void main() {
    vec2 pointPosition = (2.0 * gl_PointCoord) - 1.0;
    if (dot(pointPosition, pointPosition) > 1.0) {
        discard;
    }

    fragmentColor = vec4(0.3, 0.75, 1.0, 1.0);
}
)glsl";

constexpr std::string_view compute_shader_source = R"glsl(#version 460 core
layout(local_size_x = 256) in;

struct Particle {
    vec4 positionAge;
    vec4 velocityLifetime;
    uvec4 randomState;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

uniform uint uParticleCount;
uniform float uDeltaTime;
uniform bool uInitializeAll;
uniform uint uGlobalSeed;
uniform float uEmitterInnerRadius;
uniform float uEmitterOuterRadius;
uniform float uEmitterHalfThickness;
uniform float uMinimumLifetime;
uniform float uMaximumLifetime;
uniform float uOrbitalSpeed;
uniform float uVelocityJitter;
uniform float uEscapeRadius;
uniform float uAttractionStrength;
uniform float uSoftening;
uniform float uSwirlStrength;
uniform float uDrag;
uniform float uCoreRadius;

const uint randomStateFallback = 0x9E3779B9u;
const float twoPi = 6.28318530717958647692;
const float minimumTangentLengthSquared = 1e-12;

uint nextUint(inout uint state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

float nextUnitFloat(inout uint state) {
    return float(nextUint(state) >> 8) * (1.0 / 16777216.0);
}

uint initialRandomState(uint globalSeed, uint particleIndex) {
    uint state = globalSeed + (0x9E3779B9u * (particleIndex + 1u));
    state ^= state >> 16;
    state *= 0x7FEB352Du;
    state ^= state >> 15;
    state *= 0x846CA68Bu;
    state ^= state >> 16;
    return state == 0u ? randomStateFallback : state;
}

bool finiteFloat(float value) {
    return !isnan(value) && !isinf(value);
}

bool finiteVec3(vec3 value) {
    return !any(isnan(value)) && !any(isinf(value));
}

bool shouldRespawn(Particle particle) {
    if (!finiteVec3(particle.positionAge.xyz) ||
        !finiteVec3(particle.velocityLifetime.xyz) ||
        !finiteFloat(particle.positionAge.w) ||
        !finiteFloat(particle.velocityLifetime.w) ||
        particle.velocityLifetime.w <= 0.0 ||
        particle.positionAge.w >= particle.velocityLifetime.w) {
        return true;
    }

    float radiusSquared = dot(particle.positionAge.xyz, particle.positionAge.xyz);
    float coreRadiusSquared = uCoreRadius * uCoreRadius;
    float escapeRadiusSquared = uEscapeRadius * uEscapeRadius;
    return radiusSquared < coreRadiusSquared || radiusSquared > escapeRadiusSquared;
}

void emitParticle(inout Particle particle, inout uint randomState, bool staggerAge) {
    float angle = twoPi * nextUnitFloat(randomState);
    float innerRadiusSquared = uEmitterInnerRadius * uEmitterInnerRadius;
    float outerRadiusSquared = uEmitterOuterRadius * uEmitterOuterRadius;
    float radius = sqrt(mix(innerRadiusSquared, outerRadiusSquared,
                            nextUnitFloat(randomState)));
    float height = uEmitterHalfThickness * ((2.0 * nextUnitFloat(randomState)) - 1.0);
    float sine = sin(angle);
    float cosine = cos(angle);

    vec3 velocityJitter = uVelocityJitter * vec3(
        (2.0 * nextUnitFloat(randomState)) - 1.0,
        (2.0 * nextUnitFloat(randomState)) - 1.0,
        (2.0 * nextUnitFloat(randomState)) - 1.0);
    float lifetime = mix(uMinimumLifetime, uMaximumLifetime, nextUnitFloat(randomState));
    float age = staggerAge ? lifetime * nextUnitFloat(randomState) : 0.0;

    particle.positionAge = vec4(radius * cosine, height, radius * sine, age);
    particle.velocityLifetime =
        vec4((uOrbitalSpeed * vec3(-sine, 0.0, cosine)) + velocityJitter, lifetime);
    particle.randomState = uvec4(randomState, 0u, 0u, 0u);
}

void main() {
    uint particleIndex = gl_GlobalInvocationID.x;
    if (particleIndex >= uParticleCount) {
        return;
    }

    if (uInitializeAll) {
        Particle particle;
        uint randomState = initialRandomState(uGlobalSeed, particleIndex);
        emitParticle(particle, randomState, true);
        particles[particleIndex] = particle;
        return;
    }

    Particle particle = particles[particleIndex];
    uint randomState = particle.randomState.x;
    if (randomState == 0u) {
        randomState = randomStateFallback;
    }

    particle.positionAge.w += uDeltaTime;
    if (shouldRespawn(particle)) {
        emitParticle(particle, randomState, false);
        particles[particleIndex] = particle;
        return;
    }

    vec3 position = particle.positionAge.xyz;
    vec3 velocity = particle.velocityLifetime.xyz;

    float softenedRadiusSquared =
        dot(position, position) + (uSoftening * uSoftening);
    float inverseRadius = inversesqrt(softenedRadiusSquared);
    float inverseRadiusCubed = inverseRadius * inverseRadius * inverseRadius;
    vec3 attraction =
        -uAttractionStrength * position * inverseRadiusCubed;

    vec3 horizontalPosition = vec3(position.x, 0.0, position.z);
    vec3 tangent = cross(vec3(0.0, 1.0, 0.0), horizontalPosition);
    float tangentLengthSquared = dot(tangent, tangent);
    if (tangentLengthSquared > minimumTangentLengthSquared) {
        tangent *= inversesqrt(tangentLengthSquared);
    } else {
        tangent = vec3(0.0);
    }

    vec3 vortex = uSwirlStrength * tangent;
    vec3 drag = -uDrag * velocity;
    vec3 acceleration = attraction + vortex + drag;

    particle.velocityLifetime.xyz += acceleration * uDeltaTime;
    particle.positionAge.xyz += particle.velocityLifetime.xyz * uDeltaTime;
    if (shouldRespawn(particle)) {
        emitParticle(particle, randomState, false);
    } else {
        particle.randomState.x = randomState;
    }

    particles[particleIndex] = particle;
}
)glsl";

constexpr std::array particle_render_stages{
    ShaderStageSource{.stage = ShaderStage::vertex, .source = vertex_shader_source},
    ShaderStageSource{.stage = ShaderStage::fragment, .source = fragment_shader_source},
};

constexpr std::array particle_compute_stages{
    ShaderStageSource{.stage = ShaderStage::compute, .source = compute_shader_source},
};

constexpr GLuint compute_local_size_x = 256;
constexpr std::size_t application_particle_count_limit = 1'048'576;
constexpr float maximum_ui_point_size = 20.0F;

[[nodiscard]] ParticleSettings validated_default_particle_settings() {
    ParticleSettings settings{};
    validate_particle_settings(settings);
    return settings;
}

[[nodiscard]] GLint required_uniform_location(GLuint program, const char* name) {
    const GLint location = glGetUniformLocation(program, name);
    if (location < 0) {
        throw std::runtime_error{"Required shader uniform was optimized out: " + std::string{name}};
    }

    return location;
}

[[nodiscard]] GLsizei checked_particle_count(std::size_t particle_count) {
    if (std::cmp_greater(particle_count, std::numeric_limits<GLsizei>::max())) {
        throw std::length_error{"Particle count does not fit the glDrawArrays GLsizei limit."};
    }

    return static_cast<GLsizei>(particle_count);
}

[[nodiscard]] GLuint checked_dispatch_group_count(std::size_t particle_count) {
    const std::size_t group_count = work_group_count(particle_count, compute_local_size_x);
    if (std::cmp_greater(group_count, std::numeric_limits<GLuint>::max())) {
        throw std::length_error{"Compute work-group count does not fit GLuint."};
    }

    return static_cast<GLuint>(group_count);
}

[[nodiscard]] std::size_t query_maximum_compute_work_group_count_x() {
    GLint maximum_group_count = 0;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maximum_group_count);
    if (maximum_group_count <= 0) {
        throw std::runtime_error{
            "OpenGL returned an invalid GL_MAX_COMPUTE_WORK_GROUP_COUNT value."};
    }

    return static_cast<std::size_t>(maximum_group_count);
}

[[nodiscard]] std::size_t
maximum_supported_particle_count(ParticleBufferLimits buffer_limits,
                                 std::size_t maximum_compute_group_count) {
    const std::size_t maximum_dispatch_particle_count =
        maximum_compute_group_count > std::numeric_limits<std::size_t>::max() / compute_local_size_x
            ? std::numeric_limits<std::size_t>::max()
            : maximum_compute_group_count * compute_local_size_x;

    const std::size_t supported_count = std::min(
        {application_particle_count_limit, buffer_limits.maximum_size_bytes / sizeof(ParticleGpu),
         maximum_dispatch_particle_count,
         static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()),
         static_cast<std::size_t>(std::numeric_limits<GLuint>::max())});
    if (supported_count == 0) {
        throw std::runtime_error{"The OpenGL implementation cannot store one particle."};
    }

    return supported_count;
}

[[nodiscard]] std::optional<std::string>
editable_settings_error(ParticleSettings settings, std::size_t active_particle_count) {
    settings.particle_count = active_particle_count;
    try {
        validate_particle_settings(settings);
    } catch (const std::invalid_argument& error) {
        return std::string{error.what()};
    }

    return std::nullopt;
}

void show_tooltip(std::string_view text) {
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
        return;
    }

    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0F);
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

} // namespace

GpsDemo::GpsDemo()
    : active_settings_{validated_default_particle_settings()}, edited_settings_{active_settings_},
      particle_buffer_limits_{query_particle_buffer_limits()},
      maximum_compute_work_group_count_x_{query_maximum_compute_work_group_count_x()},
      maximum_supported_particle_count_{maximum_supported_particle_count(
          particle_buffer_limits_, maximum_compute_work_group_count_x_)},
      particle_render_program_{particle_render_stages},
      particle_compute_program_{particle_compute_stages},
      particle_buffer_{active_settings_.particle_count, particle_buffer_limits_},
      particle_count_{checked_particle_count(particle_buffer_.particle_count())},
      dispatch_group_count_{checked_dispatch_group_count(particle_buffer_.particle_count())},
      pending_particle_count_{static_cast<std::int64_t>(particle_buffer_.particle_count())} {
    try {
        glCreateVertexArrays(1, &vertex_array_);
        if (vertex_array_ == 0) {
            throw std::runtime_error{"OpenGL failed to allocate the particle vertex array."};
        }

        model_location_ = required_uniform_location(particle_render_program_.id(), "uModel");
        view_location_ = required_uniform_location(particle_render_program_.id(), "uView");
        projection_location_ =
            required_uniform_location(particle_render_program_.id(), "uProjection");
        point_size_location_ =
            required_uniform_location(particle_render_program_.id(), "uPointSize");

        const GLuint compute_program = particle_compute_program_.id();
        particle_count_location_ = required_uniform_location(compute_program, "uParticleCount");
        delta_time_location_ = required_uniform_location(compute_program, "uDeltaTime");
        initialize_all_location_ = required_uniform_location(compute_program, "uInitializeAll");
        global_seed_location_ = required_uniform_location(compute_program, "uGlobalSeed");
        emitter_inner_radius_location_ =
            required_uniform_location(compute_program, "uEmitterInnerRadius");
        emitter_outer_radius_location_ =
            required_uniform_location(compute_program, "uEmitterOuterRadius");
        emitter_half_thickness_location_ =
            required_uniform_location(compute_program, "uEmitterHalfThickness");
        minimum_lifetime_location_ = required_uniform_location(compute_program, "uMinimumLifetime");
        maximum_lifetime_location_ = required_uniform_location(compute_program, "uMaximumLifetime");
        orbital_speed_location_ = required_uniform_location(compute_program, "uOrbitalSpeed");
        velocity_jitter_location_ = required_uniform_location(compute_program, "uVelocityJitter");
        escape_radius_location_ = required_uniform_location(compute_program, "uEscapeRadius");
        attraction_strength_location_ =
            required_uniform_location(compute_program, "uAttractionStrength");
        softening_location_ = required_uniform_location(compute_program, "uSoftening");
        swirl_strength_location_ = required_uniform_location(compute_program, "uSwirlStrength");
        drag_location_ = required_uniform_location(compute_program, "uDrag");
        core_radius_location_ = required_uniform_location(compute_program, "uCoreRadius");

        dispatch_compute(0.0F, true);
    } catch (...) {
        release_vertex_array();
        throw;
    }
}

GpsDemo::~GpsDemo() {
    release_vertex_array();
}

ParticleControlEvents GpsDemo::draw_controls() {
    ParticleControlEvents events{};
    bool reset_particles_requested = false;
    bool reset_parameters_requested = false;
    bool apply_particle_count_requested = false;

    const bool settings_were_valid =
        !editable_settings_error(edited_settings_, active_settings_.particle_count).has_value();

    ImGui::SetNextWindowSize(ImVec2{430.0F, 650.0F}, ImGuiCond_FirstUseEver);
    const bool panel_visible = ImGui::Begin("Particle Laboratory");
    if (panel_visible) {
        if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Pause", &paused_);

            ImGui::BeginDisabled(!paused_);
            events.single_step_requested = ImGui::Button("Single step");
            ImGui::EndDisabled();
            show_tooltip("Advances exactly one fixed simulation step while paused.");

            ImGui::SameLine();
            ImGui::BeginDisabled(!settings_were_valid);
            reset_particles_requested = ImGui::Button("Reset particles");
            ImGui::EndDisabled();
            show_tooltip("Re-emits the complete active population from the visible seed and "
                         "emitter settings. Pause state and active count are preserved.");

            ImGui::SameLine();
            reset_parameters_requested = ImGui::Button("Reset parameters");
            show_tooltip("Restores the documented parameter defaults without replacing active "
                         "particles or changing the active count. The pending count returns to "
                         "its default and still requires Apply particle count.");

            auto seed_input = static_cast<ImU32>(edited_settings_.seed);
            if (ImGui::InputScalar("Seed", ImGuiDataType_U32, &seed_input)) {
                edited_settings_.seed = static_cast<std::uint32_t>(seed_input);
            }
            show_tooltip("The seed is reset-only: press Reset particles to rebuild the current "
                         "population from it.");

            const double fixed_step = fixed_step_accumulator_.step_seconds();
            ImGui::Text("Fixed step: %.6f s (%.0f Hz)", fixed_step, 1.0 / fixed_step);
            ImGui::Text("Last frame substeps: %zu", last_substep_count_);
        }

        if (ImGui::CollapsingHeader("Particles", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Active count: %zu", active_settings_.particle_count);
            auto pending_count_input = static_cast<ImS64>(pending_particle_count_);
            if (ImGui::InputScalar("Pending count", ImGuiDataType_S64, &pending_count_input)) {
                pending_particle_count_ = static_cast<std::int64_t>(pending_count_input);
            }
            show_tooltip("Editing this value does not allocate GPU memory. Apply explicitly to "
                         "replace the SSBO and reset the population.");

            ImGui::BeginDisabled(!settings_were_valid);
            apply_particle_count_requested = ImGui::Button("Apply particle count");
            ImGui::EndDisabled();

            const auto maximum_count = static_cast<std::int64_t>(maximum_supported_particle_count_);
            if (pending_particle_count_ < 1 || pending_particle_count_ > maximum_count) {
                const std::int64_t clamped_count =
                    std::clamp(pending_particle_count_, std::int64_t{1}, maximum_count);
                ImGui::TextColored(ImVec4{1.0F, 0.75F, 0.25F, 1.0F},
                                   "Apply will clamp this request to %lld.",
                                   static_cast<long long>(clamped_count));
            }
        }

        if (ImGui::CollapsingHeader("Emitter", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled(
                "Changes affect future respawns; Reset particles applies them all.");
            ImGui::DragFloat("Core radius", &edited_settings_.core_radius, 0.01F, 0.01F, 5.0F,
                             "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Inner radius", &edited_settings_.emitter_inner_radius, 0.01F, 0.01F,
                             10.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Outer radius", &edited_settings_.emitter_outer_radius, 0.01F, 0.01F,
                             20.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Half-thickness", &edited_settings_.emitter_half_thickness, 0.01F,
                             0.0F, 5.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Escape radius", &edited_settings_.escape_radius, 0.05F, 0.1F, 50.0F,
                             "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Minimum lifetime", &edited_settings_.minimum_lifetime, 0.05F, 0.1F,
                             60.0F, "%.2f s", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Maximum lifetime", &edited_settings_.maximum_lifetime, 0.05F, 0.1F,
                             60.0F, "%.2f s", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Orbital speed", &edited_settings_.orbital_speed, 0.01F, -5.0F, 5.0F,
                             "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Velocity jitter", &edited_settings_.velocity_jitter, 0.01F, 0.0F,
                             5.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        }

        if (ImGui::CollapsingHeader("Forces", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("Valid values are uploaded on the next compute substep.");
            ImGui::DragFloat("Attraction strength", &edited_settings_.attraction_strength, 0.01F,
                             0.01F, 20.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Softening", &edited_settings_.softening, 0.01F, 0.01F, 5.0F, "%.2f",
                             ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Swirl strength", &edited_settings_.swirl_strength, 0.01F, -2.0F, 2.0F,
                             "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Drag", &edited_settings_.drag, 0.01F, 0.0F, 5.0F, "%.2f",
                             ImGuiSliderFlags_AlwaysClamp);
        }

        if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat("Point size", &edited_settings_.point_size, 0.1F, 1.0F,
                             maximum_ui_point_size, "%.1f px", ImGuiSliderFlags_AlwaysClamp);
            show_tooltip("This controls the current diagnostic GL_POINTS renderer immediately. "
                         "Feature 7 replaces points with billboards.");
        }
    }

    if (reset_parameters_requested) {
        reset_parameters();
    }

    std::optional<std::string> validation_error =
        editable_settings_error(edited_settings_, active_settings_.particle_count);
    if (!validation_error.has_value()) {
        const std::size_t active_particle_count = active_settings_.particle_count;
        active_settings_ = edited_settings_;
        active_settings_.particle_count = active_particle_count;
    }

    if (apply_particle_count_requested) {
        if (validation_error.has_value()) {
            last_action_failed_ = true;
            last_action_status_ = "Particle count was not applied because the edited settings "
                                  "are invalid.";
        } else {
            const std::int64_t requested_count = pending_particle_count_;
            try {
                const bool buffer_recreated = apply_pending_particle_count();
                events.particle_state_replaced = buffer_recreated;
                last_action_failed_ = false;
                if (buffer_recreated) {
                    last_action_status_ = "Particle count applied; the SSBO and population were "
                                          "recreated.";
                } else {
                    last_action_status_ =
                        "The active count already matched; the SSBO was left unchanged.";
                }
                if (last_particle_count_was_clamped_) {
                    last_action_status_ += " Requested " + std::to_string(requested_count) +
                                           ", applied " + std::to_string(pending_particle_count_) +
                                           '.';
                }
            } catch (const std::exception& error) {
                last_action_failed_ = true;
                last_action_status_ = "Particle count apply failed: " + std::string{error.what()};
            }
        }
    }

    if (reset_particles_requested) {
        if (validation_error.has_value()) {
            last_action_failed_ = true;
            last_action_status_ =
                "Particles were not reset because the edited settings are invalid.";
        } else {
            reset_particles();
            events.particle_state_replaced = true;
            last_action_failed_ = false;
            last_action_status_ =
                "Particles reset from the visible seed; parameters and pause state were preserved.";
        }
    }

    if (panel_visible && ImGui::CollapsingHeader("Diagnostics", ImGuiTreeNodeFlags_DefaultOpen)) {
        const double buffer_size_mib =
            (static_cast<double>(active_settings_.particle_count) * sizeof(ParticleGpu)) /
            (1024.0 * 1024.0);
        ImGui::Text("Work groups per substep: %u", dispatch_group_count_);
        ImGui::Text("Particle buffer: %.3f MiB", buffer_size_mib);
        ImGui::Text("Supported count: 1 - %zu (%s limit)", maximum_supported_particle_count_,
                    maximum_supported_particle_count_ == application_particle_count_limit
                        ? "application"
                        : "hardware");

        const float frame_rate = ImGui::GetIO().Framerate;
        if (frame_rate > 0.0F) {
            ImGui::Text("Rendered frame: %.2f ms (%.1f FPS)", 1000.0F / frame_rate, frame_rate);
        } else {
            ImGui::TextUnformatted("Rendered frame: unavailable");
        }

        if (has_particle_count_apply_result_) {
            ImGui::Text("Last count request clamped: %s",
                        last_particle_count_was_clamped_ ? "yes" : "no");
        } else {
            ImGui::TextUnformatted("Last count request clamped: not applied yet");
        }

        if (validation_error.has_value()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 0.35F, 0.35F, 1.0F});
            ImGui::TextWrapped("Invalid settings: %s", validation_error->c_str());
            ImGui::PopStyleColor();
            ImGui::TextDisabled("The simulation continues with the last valid values.");
        } else {
            ImGui::TextColored(ImVec4{0.45F, 0.9F, 0.55F, 1.0F}, "Edited settings are active.");
        }

        if (!last_action_status_.empty()) {
            const ImVec4 status_color = last_action_failed_ ? ImVec4{1.0F, 0.35F, 0.35F, 1.0F}
                                                            : ImVec4{0.45F, 0.9F, 0.55F, 1.0F};
            ImGui::PushStyleColor(ImGuiCol_Text, status_color);
            ImGui::TextWrapped("Last action: %s", last_action_status_.c_str());
            ImGui::PopStyleColor();
        }
    }

    ImGui::End();

    return events;
}

void GpsDemo::update(double frame_delta_seconds, bool single_step_requested) noexcept {
    last_substep_count_ = fixed_step_accumulator_.advance(frame_delta_seconds, paused_);
    const auto fixed_step_seconds = static_cast<float>(fixed_step_accumulator_.step_seconds());

    for (std::size_t substep = 0; substep < last_substep_count_; ++substep) {
        dispatch_compute(fixed_step_seconds, false);
    }

    if (paused_ && single_step_requested) {
        step_simulation_once();
        last_substep_count_ = 1;
    }
}

void GpsDemo::step_simulation_once() noexcept {
    dispatch_compute(static_cast<float>(fixed_step_accumulator_.step_seconds()), false);
}

void GpsDemo::draw(int framebuffer_width, int framebuffer_height) noexcept {
    if (framebuffer_width <= 0 || framebuffer_height <= 0) {
        return;
    }

    const glm::mat4 model{1.0F};
    const glm::mat4 view =
        glm::lookAt(glm::vec3{0.0F, 4.5F, 6.5F}, glm::vec3{0.0F}, glm::vec3{0.0F, 1.0F, 0.0F});
    const float aspect_ratio =
        static_cast<float>(framebuffer_width) / static_cast<float>(framebuffer_height);
    const glm::mat4 projection = glm::perspective(glm::radians(45.0F), aspect_ratio, 0.1F, 100.0F);

    glUseProgram(particle_render_program_.id());
    glUniformMatrix4fv(model_location_, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(view_location_, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projection_location_, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(point_size_location_, active_settings_.point_size);
    glBindVertexArray(vertex_array_);
    glDrawArrays(GL_POINTS, 0, particle_count_);
}

void GpsDemo::dispatch_compute(float delta_time, bool initialize_all) noexcept {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, particle_buffer_binding_index,
                     particle_buffer_.id());

    glUseProgram(particle_compute_program_.id());
    glUniform1ui(particle_count_location_, static_cast<GLuint>(particle_count_));
    glUniform1f(delta_time_location_, delta_time);
    glUniform1i(initialize_all_location_, initialize_all ? GL_TRUE : GL_FALSE);
    glUniform1ui(global_seed_location_, static_cast<GLuint>(active_settings_.seed));
    glUniform1f(emitter_inner_radius_location_, active_settings_.emitter_inner_radius);
    glUniform1f(emitter_outer_radius_location_, active_settings_.emitter_outer_radius);
    glUniform1f(emitter_half_thickness_location_, active_settings_.emitter_half_thickness);
    glUniform1f(minimum_lifetime_location_, active_settings_.minimum_lifetime);
    glUniform1f(maximum_lifetime_location_, active_settings_.maximum_lifetime);
    glUniform1f(orbital_speed_location_, active_settings_.orbital_speed);
    glUniform1f(velocity_jitter_location_, active_settings_.velocity_jitter);
    glUniform1f(escape_radius_location_, active_settings_.escape_radius);
    glUniform1f(attraction_strength_location_, active_settings_.attraction_strength);
    glUniform1f(softening_location_, active_settings_.softening);
    glUniform1f(swirl_strength_location_, active_settings_.swirl_strength);
    glUniform1f(drag_location_, active_settings_.drag);
    glUniform1f(core_radius_location_, active_settings_.core_radius);
    glDispatchCompute(dispatch_group_count_, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

bool GpsDemo::apply_pending_particle_count() {
    const auto maximum_count = static_cast<std::int64_t>(maximum_supported_particle_count_);
    const std::int64_t clamped_count =
        std::clamp(pending_particle_count_, std::int64_t{1}, maximum_count);
    const bool count_was_clamped = clamped_count != pending_particle_count_;
    const auto particle_count = static_cast<std::size_t>(clamped_count);
    const bool buffer_recreated = particle_count != active_settings_.particle_count;

    if (buffer_recreated) {
        recreate_particle_buffer(particle_count);
    }

    pending_particle_count_ = clamped_count;
    has_particle_count_apply_result_ = true;
    last_particle_count_was_clamped_ = count_was_clamped;
    return buffer_recreated;
}

void GpsDemo::recreate_particle_buffer(std::size_t particle_count) {
    const GLsizei checked_count = checked_particle_count(particle_count);
    const GLuint checked_group_count = checked_dispatch_group_count(particle_count);
    if (std::cmp_greater(checked_group_count, maximum_compute_work_group_count_x_)) {
        throw std::length_error{
            "Particle count exceeds GL_MAX_COMPUTE_WORK_GROUP_COUNT for this shader."};
    }

    ParticleBuffer replacement{particle_count, particle_buffer_limits_};
    particle_buffer_ = std::move(replacement);
    active_settings_.particle_count = particle_count;
    edited_settings_.particle_count = particle_count;
    particle_count_ = checked_count;
    dispatch_group_count_ = checked_group_count;
    reset_particles();
}

void GpsDemo::reset_parameters() {
    ParticleSettings defaults = validated_default_particle_settings();
    pending_particle_count_ = static_cast<std::int64_t>(defaults.particle_count);
    defaults.particle_count = active_settings_.particle_count;
    edited_settings_ = defaults;
    last_action_failed_ = false;
    last_action_status_ = "Parameter defaults restored; active particles and count were unchanged.";
}

void GpsDemo::reset_particles() noexcept {
    fixed_step_accumulator_.reset();
    dispatch_compute(0.0F, true);
}

void GpsDemo::release_vertex_array() noexcept {
    if (vertex_array_ != 0) {
        glDeleteVertexArrays(1, &vertex_array_);
        vertex_array_ = 0;
    }
}
} // namespace gps

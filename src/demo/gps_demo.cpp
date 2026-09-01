#include "demo/gps_demo.hpp"

#include "simulation/work_group_count.hpp"

#include <array>
#include <cstddef>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

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

void main() {
    uint particleIndex = uint(gl_VertexID);
    Particle particle = particles[particleIndex];

    gl_Position = uProjection * uView * uModel * vec4(particle.positionAge.xyz, 1.0);
    gl_PointSize = 3.0;
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

const uint randomStateFallback = 0x9E3779B9u;
const float twoPi = 6.28318530717958647692;

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

    float escapeRadiusSquared = uEscapeRadius * uEscapeRadius;
    return dot(particle.positionAge.xyz, particle.positionAge.xyz) > escapeRadiusSquared;
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
    if (particle_count > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::length_error{"Particle count does not fit the glDrawArrays GLsizei limit."};
    }

    return static_cast<GLsizei>(particle_count);
}

[[nodiscard]] GLuint checked_dispatch_group_count(std::size_t particle_count) {
    const std::size_t group_count = work_group_count(particle_count, compute_local_size_x);
    if (group_count > static_cast<std::size_t>(std::numeric_limits<GLuint>::max())) {
        throw std::length_error{"Compute work-group count does not fit GLuint."};
    }

    return static_cast<GLuint>(group_count);
}

} // namespace

GpsDemo::GpsDemo()
    : particle_settings_{validated_default_particle_settings()},
      particle_render_program_{particle_render_stages},
      particle_compute_program_{particle_compute_stages},
      particle_buffer_{particle_settings_.particle_count},
      particle_count_{checked_particle_count(particle_buffer_.particle_count())},
      dispatch_group_count_{checked_dispatch_group_count(particle_buffer_.particle_count())} {
    try {
        glCreateVertexArrays(1, &vertex_array_);
        if (vertex_array_ == 0) {
            throw std::runtime_error{"OpenGL failed to allocate the particle vertex array."};
        }

        model_location_ = required_uniform_location(particle_render_program_.id(), "uModel");
        view_location_ = required_uniform_location(particle_render_program_.id(), "uView");
        projection_location_ =
            required_uniform_location(particle_render_program_.id(), "uProjection");

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
    if (ImGui::Begin("Particles")) {
        ImGui::InputScalar("Seed", ImGuiDataType_U32, &particle_settings_.seed);
        events.particles_reset = ImGui::Button("Reset particles");
    }
    ImGui::End();

    if (events.particles_reset) {
        reset_particles();
    }

    return events;
}

void GpsDemo::draw(int framebuffer_width, int framebuffer_height, float delta_time) noexcept {
    if (framebuffer_width <= 0 || framebuffer_height <= 0) {
        return;
    }

    const glm::mat4 model{1.0F};
    const glm::mat4 view =
        glm::lookAt(glm::vec3{0.0F, 4.5F, 6.5F}, glm::vec3{0.0F}, glm::vec3{0.0F, 1.0F, 0.0F});
    const float aspect_ratio =
        static_cast<float>(framebuffer_width) / static_cast<float>(framebuffer_height);
    const glm::mat4 projection = glm::perspective(glm::radians(45.0F), aspect_ratio, 0.1F, 100.0F);

    dispatch_compute(delta_time, false);

    glUseProgram(particle_render_program_.id());
    glUniformMatrix4fv(model_location_, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(view_location_, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projection_location_, 1, GL_FALSE, glm::value_ptr(projection));
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
    glUniform1ui(global_seed_location_, static_cast<GLuint>(particle_settings_.seed));
    glUniform1f(emitter_inner_radius_location_, particle_settings_.emitter_inner_radius);
    glUniform1f(emitter_outer_radius_location_, particle_settings_.emitter_outer_radius);
    glUniform1f(emitter_half_thickness_location_, particle_settings_.emitter_half_thickness);
    glUniform1f(minimum_lifetime_location_, particle_settings_.minimum_lifetime);
    glUniform1f(maximum_lifetime_location_, particle_settings_.maximum_lifetime);
    glUniform1f(orbital_speed_location_, particle_settings_.orbital_speed);
    glUniform1f(velocity_jitter_location_, particle_settings_.velocity_jitter);
    glUniform1f(escape_radius_location_, particle_settings_.escape_radius);
    glDispatchCompute(dispatch_group_count_, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GpsDemo::reset_particles() {
    validate_particle_settings(particle_settings_);
    dispatch_compute(0.0F, true);
}

void GpsDemo::release_vertex_array() noexcept {
    if (vertex_array_ != 0) {
        glDeleteVertexArrays(1, &vertex_array_);
        vertex_array_ = 0;
    }
}
} // namespace gps

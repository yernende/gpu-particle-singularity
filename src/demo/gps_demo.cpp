#include "demo/gps_demo.hpp"

#include "simulation/particle_initializer.hpp"
#include "simulation/work_group_count.hpp"

#include <array>
#include <cstddef>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <limits>
#include <stdexcept>
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

void main() {
    uint particleIndex = gl_GlobalInvocationID.x;
    if (particleIndex >= uParticleCount) {
        return;
    }

    particles[particleIndex].positionAge.xyz +=
        particles[particleIndex].velocityLifetime.xyz * uDeltaTime;
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
    : particle_render_program_{particle_render_stages},
      particle_compute_program_{particle_compute_stages},
      particle_buffer_{generate_particles(ParticleInitializationSettings{})},
      particle_count_{checked_particle_count(particle_buffer_.particle_count())},
      dispatch_group_count_{checked_dispatch_group_count(particle_buffer_.particle_count())} {
    try {
        glCreateVertexArrays(1, &vertex_array_);
        if (vertex_array_ == 0) {
            throw std::runtime_error{"OpenGL failed to allocate the particle vertex array."};
        }

        model_location_ = glGetUniformLocation(particle_render_program_.id(), "uModel");
        view_location_ = glGetUniformLocation(particle_render_program_.id(), "uView");
        projection_location_ = glGetUniformLocation(particle_render_program_.id(), "uProjection");
        if (model_location_ < 0 || view_location_ < 0 || projection_location_ < 0) {
            throw std::runtime_error{"Required particle shader uniforms were optimized out."};
        }

        particle_count_location_ =
            glGetUniformLocation(particle_compute_program_.id(), "uParticleCount");
        delta_time_location_ = glGetUniformLocation(particle_compute_program_.id(), "uDeltaTime");
        if (particle_count_location_ < 0 || delta_time_location_ < 0) {
            throw std::runtime_error{"Required compute shader uniforms were optimized out."};
        }
    } catch (...) {
        release_vertex_array();
        throw;
    }
}

GpsDemo::~GpsDemo() {
    release_vertex_array();
}

void GpsDemo::draw(int framebuffer_width, int framebuffer_height, float delta_time) const noexcept {
    if (framebuffer_width <= 0 || framebuffer_height <= 0) {
        return;
    }

    const glm::mat4 model{1.0F};
    const glm::mat4 view =
        glm::lookAt(glm::vec3{0.0F, 4.5F, 6.5F}, glm::vec3{0.0F}, glm::vec3{0.0F, 1.0F, 0.0F});
    const float aspect_ratio =
        static_cast<float>(framebuffer_width) / static_cast<float>(framebuffer_height);
    const glm::mat4 projection = glm::perspective(glm::radians(45.0F), aspect_ratio, 0.1F, 100.0F);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, particle_buffer_binding_index,
                     particle_buffer_.id());

    glUseProgram(particle_compute_program_.id());
    glUniform1ui(particle_count_location_, static_cast<GLuint>(particle_count_));
    glUniform1f(delta_time_location_, delta_time);
    glDispatchCompute(dispatch_group_count_, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(particle_render_program_.id());
    glUniformMatrix4fv(model_location_, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(view_location_, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projection_location_, 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(vertex_array_);
    glDrawArrays(GL_POINTS, 0, particle_count_);
}

void GpsDemo::release_vertex_array() noexcept {
    if (vertex_array_ != 0) {
        glDeleteVertexArrays(1, &vertex_array_);
        vertex_array_ = 0;
    }
}
} // namespace gps

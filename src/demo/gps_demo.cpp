#include "demo/gps_demo.hpp"

#include "simulation/particle_initializer.hpp"

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

[[nodiscard]] GLsizei checked_particle_count(std::size_t particle_count) {
    if (particle_count > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::length_error{"Particle count does not fit the glDrawArrays GLsizei limit."};
    }

    return static_cast<GLsizei>(particle_count);
}

} // namespace

GpsDemo::GpsDemo()
    : program_{vertex_shader_source, fragment_shader_source},
      particle_buffer_{generate_particles(ParticleInitializationSettings{})},
      particle_count_{checked_particle_count(particle_buffer_.particle_count())} {
    try {
        glCreateVertexArrays(1, &vertex_array_);
        if (vertex_array_ == 0) {
            throw std::runtime_error{"OpenGL failed to allocate the particle vertex array."};
        }

        model_location_ = glGetUniformLocation(program_.id(), "uModel");
        view_location_ = glGetUniformLocation(program_.id(), "uView");
        projection_location_ = glGetUniformLocation(program_.id(), "uProjection");
        if (model_location_ < 0 || view_location_ < 0 || projection_location_ < 0) {
            throw std::runtime_error{"Required particle shader uniforms were optimized out."};
        }
    } catch (...) {
        release_vertex_array();
        throw;
    }
}

GpsDemo::~GpsDemo() {
    release_vertex_array();
}

void GpsDemo::draw(int framebuffer_width, int framebuffer_height) const noexcept {
    if (framebuffer_width <= 0 || framebuffer_height <= 0) {
        return;
    }

    const glm::mat4 model{1.0F};
    const glm::mat4 view =
        glm::lookAt(glm::vec3{0.0F, 4.5F, 6.5F}, glm::vec3{0.0F}, glm::vec3{0.0F, 1.0F, 0.0F});
    const float aspect_ratio =
        static_cast<float>(framebuffer_width) / static_cast<float>(framebuffer_height);
    const glm::mat4 projection = glm::perspective(glm::radians(45.0F), aspect_ratio, 0.1F, 100.0F);

    glUseProgram(program_.id());
    glUniformMatrix4fv(model_location_, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(view_location_, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projection_location_, 1, GL_FALSE, glm::value_ptr(projection));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, particle_buffer_binding_index,
                     particle_buffer_.id());
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

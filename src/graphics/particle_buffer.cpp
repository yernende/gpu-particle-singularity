#include "graphics/particle_buffer.hpp"

#include <limits>
#include <stdexcept>
#include <string>

namespace gps {
namespace {

[[nodiscard]] constexpr GLsizeiptr particle_buffer_size(std::size_t particle_count) {
    if (particle_count == 0) {
        throw std::invalid_argument{"A particle buffer requires at least one particle."};
    }

    constexpr std::size_t particle_size = sizeof(ParticleGpu);
    if (particle_count > std::numeric_limits<std::size_t>::max() / particle_size) {
        throw std::length_error{"Particle buffer byte count overflows std::size_t."};
    }

    const std::size_t byte_count = particle_count * particle_size;
    if (byte_count > static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())) {
        throw std::length_error{"Particle buffer byte count does not fit GLsizeiptr."};
    }

    return static_cast<GLsizeiptr>(byte_count);
}

void validate_implementation_limit(GLsizeiptr byte_count) {
    GLint64 maximum_buffer_size = 0;
    glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maximum_buffer_size);
    if (maximum_buffer_size <= 0) {
        throw std::runtime_error{
            "OpenGL returned an invalid GL_MAX_SHADER_STORAGE_BLOCK_SIZE value."};
    }

    if (static_cast<GLint64>(byte_count) > maximum_buffer_size) {
        throw std::length_error{"Particle buffer requires " + std::to_string(byte_count) +
                                " bytes, but GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " +
                                std::to_string(maximum_buffer_size) + " bytes."};
    }
}

} // namespace

ParticleBuffer::ParticleBuffer(std::span<const ParticleGpu> particles)
    : particle_count_{particles.size()} {
    const GLsizeiptr byte_count = particle_buffer_size(particle_count_);
    validate_implementation_limit(byte_count);

    glCreateBuffers(1, &buffer_);
    if (buffer_ == 0) {
        throw std::runtime_error{"OpenGL failed to allocate the particle buffer."};
    }

    glNamedBufferStorage(buffer_, byte_count, particles.data(), GL_DYNAMIC_STORAGE_BIT);
}

ParticleBuffer::~ParticleBuffer() {
    if (buffer_ != 0) {
        glDeleteBuffers(1, &buffer_);
    }
}

GLuint ParticleBuffer::id() const noexcept {
    return buffer_;
}

std::size_t ParticleBuffer::particle_count() const noexcept {
    return particle_count_;
}

} // namespace gps

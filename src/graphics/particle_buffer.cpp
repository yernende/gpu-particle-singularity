#include "graphics/particle_buffer.hpp"

#include "graphics/particle_gpu.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

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

    if (std::cmp_greater(byte_count, std::numeric_limits<GLsizeiptr>::max())) {
        throw std::length_error{"Particle buffer byte count does not fit GLsizeiptr."};
    }

    return static_cast<GLsizeiptr>(byte_count);
}

void validate_implementation_limit(GLsizeiptr byte_count, ParticleBufferLimits limits) {
    if (limits.maximum_size_bytes == 0) {
        throw std::invalid_argument{"Maximum particle-buffer size must be greater than zero."};
    }

    if (std::cmp_greater(byte_count, limits.maximum_size_bytes)) {
        throw std::length_error{"Particle buffer requires " + std::to_string(byte_count) +
                                " bytes, but GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " +
                                std::to_string(limits.maximum_size_bytes) + " bytes."};
    }
}

} // namespace

ParticleBufferLimits query_particle_buffer_limits() {
    GLint64 maximum_buffer_size = 0;
    glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maximum_buffer_size);
    if (maximum_buffer_size <= 0) {
        throw std::runtime_error{
            "OpenGL returned an invalid GL_MAX_SHADER_STORAGE_BLOCK_SIZE value."};
    }

    if (std::cmp_greater(maximum_buffer_size, std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error{
            "GL_MAX_SHADER_STORAGE_BLOCK_SIZE does not fit the CPU size type."};
    }

    return ParticleBufferLimits{static_cast<std::size_t>(maximum_buffer_size)};
}

ParticleBuffer::ParticleBuffer(std::size_t particle_count, ParticleBufferLimits limits)
    : particle_count_{particle_count} {
    const GLsizeiptr byte_count = particle_buffer_size(particle_count_);
    validate_implementation_limit(byte_count, limits);

    glCreateBuffers(1, &buffer_);
    if (buffer_ == 0) {
        throw std::runtime_error{"OpenGL failed to allocate the particle buffer."};
    }

    glNamedBufferStorage(buffer_, byte_count, nullptr, 0);
}

ParticleBuffer::~ParticleBuffer() {
    release();
}

ParticleBuffer::ParticleBuffer(ParticleBuffer&& other) noexcept
    : buffer_{std::exchange(other.buffer_, 0)},
      particle_count_{std::exchange(other.particle_count_, 0)} {}

ParticleBuffer& ParticleBuffer::operator=(ParticleBuffer&& other) noexcept {
    if (this != &other) {
        release();
        buffer_ = std::exchange(other.buffer_, 0);
        particle_count_ = std::exchange(other.particle_count_, 0);
    }

    return *this;
}

void ParticleBuffer::release() noexcept {
    if (buffer_ != 0) {
        glDeleteBuffers(1, &buffer_);
        buffer_ = 0;
    }
    particle_count_ = 0;
}

GLuint ParticleBuffer::id() const noexcept {
    return buffer_;
}

std::size_t ParticleBuffer::particle_count() const noexcept {
    return particle_count_;
}

} // namespace gps

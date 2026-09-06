#pragma once

#include <cstddef>
#include <glad/gl.h>

namespace gps {

inline constexpr GLuint particle_buffer_binding_index = 0;

struct ParticleBufferLimits {
    std::size_t maximum_size_bytes{0};
};

// Query once after OpenGL is loaded, then reuse the result for every buffer recreation.
[[nodiscard]] ParticleBufferLimits query_particle_buffer_limits();

// Construction and destruction require a current OpenGL context.
class ParticleBuffer final {
  public:
    ParticleBuffer(std::size_t particle_count, ParticleBufferLimits limits);
    ~ParticleBuffer();

    ParticleBuffer(const ParticleBuffer&) = delete;
    ParticleBuffer& operator=(const ParticleBuffer&) = delete;
    ParticleBuffer(ParticleBuffer&& other) noexcept;
    ParticleBuffer& operator=(ParticleBuffer&& other) noexcept;

    [[nodiscard]] GLuint id() const noexcept;
    [[nodiscard]] std::size_t particle_count() const noexcept;

  private:
    void release() noexcept;

    GLuint buffer_{0};
    std::size_t particle_count_{0};
};

} // namespace gps

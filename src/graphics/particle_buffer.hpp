#pragma once

#include <cstddef>
#include <glad/gl.h>

namespace gps {

inline constexpr GLuint particle_buffer_binding_index = 0;

// Construction and destruction require a current OpenGL context.
class ParticleBuffer final {
  public:
    explicit ParticleBuffer(std::size_t particle_count);
    ~ParticleBuffer();

    ParticleBuffer(const ParticleBuffer&) = delete;
    ParticleBuffer& operator=(const ParticleBuffer&) = delete;
    ParticleBuffer(ParticleBuffer&&) = delete;
    ParticleBuffer& operator=(ParticleBuffer&&) = delete;

    [[nodiscard]] GLuint id() const noexcept;
    [[nodiscard]] std::size_t particle_count() const noexcept;

  private:
    GLuint buffer_{0};
    std::size_t particle_count_{0};
};

} // namespace gps

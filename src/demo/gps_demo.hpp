#pragma once

#include "graphics/particle_buffer.hpp"
#include "graphics/shader_program.hpp"

#include <glad/gl.h>

namespace gps {
class GpsDemo final {
  public:
    GpsDemo();
    ~GpsDemo();

    GpsDemo(const GpsDemo&) = delete;
    GpsDemo& operator=(const GpsDemo&) = delete;
    GpsDemo(GpsDemo&&) = delete;
    GpsDemo& operator=(GpsDemo&&) = delete;

    void draw(int framebuffer_width, int framebuffer_height) const noexcept;

  private:
    void release_vertex_array() noexcept;

    ShaderProgram program_;
    ParticleBuffer particle_buffer_;
    GLuint vertex_array_{0};
    GLint model_location_{-1};
    GLint view_location_{-1};
    GLint projection_location_{-1};
    GLsizei particle_count_{0};
};
} // namespace gps

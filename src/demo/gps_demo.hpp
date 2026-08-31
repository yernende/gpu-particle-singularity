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

    void draw(int framebuffer_width, int framebuffer_height, float delta_time) const noexcept;

  private:
    void release_vertex_array() noexcept;

    ShaderProgram particle_render_program_;
    ShaderProgram particle_compute_program_;
    ParticleBuffer particle_buffer_;
    GLuint vertex_array_{0};
    GLint model_location_{-1};
    GLint view_location_{-1};
    GLint projection_location_{-1};
    GLint particle_count_location_{-1};
    GLint delta_time_location_{-1};
    GLsizei particle_count_{0};
    GLuint dispatch_group_count_{0};
};
} // namespace gps

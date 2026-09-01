#pragma once

#include "graphics/particle_buffer.hpp"
#include "graphics/shader_program.hpp"
#include "simulation/particle_settings.hpp"

#include <glad/gl.h>

namespace gps {
struct ParticleControlEvents {
    bool particles_reset{false};
};

class GpsDemo final {
  public:
    GpsDemo();
    ~GpsDemo();

    GpsDemo(const GpsDemo&) = delete;
    GpsDemo& operator=(const GpsDemo&) = delete;
    GpsDemo(GpsDemo&&) = delete;
    GpsDemo& operator=(GpsDemo&&) = delete;

    [[nodiscard]] ParticleControlEvents draw_controls();
    void draw(int framebuffer_width, int framebuffer_height, float delta_time) noexcept;

  private:
    void dispatch_compute(float delta_time, bool initialize_all) noexcept;
    void release_vertex_array() noexcept;
    void reset_particles();

    ParticleSettings particle_settings_{};
    ShaderProgram particle_render_program_;
    ShaderProgram particle_compute_program_;
    ParticleBuffer particle_buffer_;
    GLuint vertex_array_{0};
    GLint model_location_{-1};
    GLint view_location_{-1};
    GLint projection_location_{-1};
    GLint particle_count_location_{-1};
    GLint delta_time_location_{-1};
    GLint initialize_all_location_{-1};
    GLint global_seed_location_{-1};
    GLint emitter_inner_radius_location_{-1};
    GLint emitter_outer_radius_location_{-1};
    GLint emitter_half_thickness_location_{-1};
    GLint minimum_lifetime_location_{-1};
    GLint maximum_lifetime_location_{-1};
    GLint orbital_speed_location_{-1};
    GLint velocity_jitter_location_{-1};
    GLint escape_radius_location_{-1};
    GLsizei particle_count_{0};
    GLuint dispatch_group_count_{0};
};
} // namespace gps

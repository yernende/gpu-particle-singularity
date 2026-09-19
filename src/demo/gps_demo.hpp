#pragma once

#include "graphics/particle_buffer.hpp"
#include "graphics/shader_program.hpp"
#include "simulation/fixed_step_accumulator.hpp"
#include "simulation/particle_settings.hpp"

#include <cstddef>
#include <cstdint>
#include <glad/gl.h>
#include <string>

namespace gps {
struct ParticleControlEvents {
    bool particle_state_replaced{false};
    bool single_step_requested{false};
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
    void update(double frame_delta_seconds, bool single_step_requested) noexcept;
    void step_simulation_once() noexcept;
    void draw(int framebuffer_width, int framebuffer_height) noexcept;

  private:
    [[nodiscard]] bool apply_pending_particle_count();
    void dispatch_compute(float delta_time, bool initialize_all) noexcept;
    void recreate_particle_buffer(std::size_t particle_count);
    void release_vertex_array() noexcept;
    void reset_parameters();
    void reset_particles() noexcept;

    ParticleSettings active_settings_{};
    ParticleSettings edited_settings_{};
    ParticleBufferLimits particle_buffer_limits_{};
    std::size_t maximum_compute_work_group_count_x_{0};
    std::size_t maximum_supported_particle_count_{0};
    ShaderProgram particle_render_program_;
    ShaderProgram particle_compute_program_;
    ParticleBuffer particle_buffer_;
    FixedStepAccumulator fixed_step_accumulator_{};
    GLuint vertex_array_{0};
    GLint model_location_{-1};
    GLint view_location_{-1};
    GLint projection_location_{-1};
    GLint point_size_location_{-1};
    GLint diagnostic_points_location_{-1};
    GLint billboard_base_size_location_{-1};
    GLint birth_color_location_{-1};
    GLint death_color_location_{-1};
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
    GLint attraction_strength_location_{-1};
    GLint softening_location_{-1};
    GLint swirl_strength_location_{-1};
    GLint drag_location_{-1};
    GLint core_radius_location_{-1};
    GLsizei particle_count_{0};
    GLuint dispatch_group_count_{0};
    std::int64_t pending_particle_count_{0};
    std::size_t last_substep_count_{0};
    bool paused_{false};
    bool has_particle_count_apply_result_{false};
    bool last_particle_count_was_clamped_{false};
    bool last_action_failed_{false};
    std::string last_action_status_{};
};
} // namespace gps

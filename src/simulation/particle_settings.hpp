#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gps {
inline constexpr float minimum_emitter_core_margin = 0.05F;

struct ParticleSettings {
    std::size_t particle_count = 4'097;
    std::uint32_t seed = 12'345U;
    float emitter_inner_radius = 1.4F;
    float emitter_outer_radius = 3.2F;
    float emitter_half_thickness = 0.2F;
    float minimum_lifetime = 6.0F;
    float maximum_lifetime = 18.0F;
    float orbital_speed = 1.25F;
    float velocity_jitter = 0.08F;
    float escape_radius = 8.0F;
    float attraction_strength = 3.0F;
    float softening = 0.30F;
    float swirl_strength = 0.04F;
    float drag = 0.08F;
    float core_radius = 0.45F;
    float billboard_base_size = 0.035F; // Half-size in world units before the lifetime scale.
    std::array<float, 3> birth_color{0.20F, 0.55F, 1.0F};
    std::array<float, 3> death_color{1.0F, 0.25F, 0.05F};
    bool diagnostic_points = false;
    float point_size = 3.0F;
};

void validate_particle_settings(const ParticleSettings& settings);
} // namespace gps

#pragma once

#include <cstddef>
#include <cstdint>

namespace gps {
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
};

void validate_particle_settings(const ParticleSettings& settings);
} // namespace gps

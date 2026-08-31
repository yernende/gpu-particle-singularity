#pragma once

#include "graphics/particle_gpu.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gps {
struct ParticleInitializationSettings {
    std::size_t particle_count = 4'096;
    std::uint32_t seed = 12'345U;
    float inner_radius = 1.4F;
    float outer_radius = 3.2F;
    float half_thickness = 0.2F;
    float minimum_lifetime = 6.0F;
    float maximum_lifetime = 18.0F;
    float initial_orbital_speed = 0.35F;
};

[[nodiscard]] std::vector<ParticleGpu>
generate_particles(const ParticleInitializationSettings& settings);
} // namespace gps

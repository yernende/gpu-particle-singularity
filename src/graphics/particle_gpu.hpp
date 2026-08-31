#pragma once

#include <cstddef>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint4.hpp>
#include <type_traits>

namespace gps {

/**
 * Interpret the fields as:
 * position_age.xyz       world-space position
 * position_age.w         age in seconds
 * velocity_lifetime.xyz  world-space velocity
 * velocity_lifetime.w    lifetime in seconds
 * random_state.x         nonzero persistent random state
 * random_state.yzw       reserved and initialized to zero
 */
struct ParticleGpu {
    glm::vec4 position_age;
    glm::vec4 velocity_lifetime;
    glm::uvec4 random_state;
};

static_assert(std::is_standard_layout_v<ParticleGpu>);
static_assert(std::is_trivially_copyable_v<ParticleGpu>);
static_assert(sizeof(ParticleGpu) == 48);
static_assert(offsetof(ParticleGpu, position_age) == 0);
static_assert(offsetof(ParticleGpu, velocity_lifetime) == 16);
static_assert(offsetof(ParticleGpu, random_state) == 32);
} // namespace gps

#include "simulation/particle_initializer.hpp"

#include "simulation/xorshift32.hpp"

#include <cmath>
#include <glm/ext/vector_float3.hpp>
#include <numbers>
#include <stdexcept>

namespace gps {
namespace {
void validate_settings(const ParticleInitializationSettings& settings) {
    if (!std::isfinite(settings.inner_radius) || settings.inner_radius <= 0.0F) {
        throw std::invalid_argument("Particle inner radius must be finite and greater than zero");
    }
    if (!std::isfinite(settings.outer_radius) || settings.outer_radius < settings.inner_radius) {
        throw std::invalid_argument(
            "Particle outer radius must be finite and at least the inner radius");
    }
    if (!std::isfinite(settings.half_thickness) || settings.half_thickness < 0.0F) {
        throw std::invalid_argument("Particle half-thickness must be finite and non-negative");
    }
    if (!std::isfinite(settings.minimum_lifetime) || settings.minimum_lifetime <= 0.0F) {
        throw std::invalid_argument(
            "Particle minimum lifetime must be finite and greater than zero");
    }
    if (!std::isfinite(settings.maximum_lifetime) ||
        settings.maximum_lifetime < settings.minimum_lifetime) {
        throw std::invalid_argument(
            "Particle maximum lifetime must be finite and at least the minimum lifetime");
    }
    if (!std::isfinite(settings.initial_orbital_speed)) {
        throw std::invalid_argument("Particle initial orbital speed must be finite");
    }
}
} // namespace

std::vector<ParticleGpu> generate_particles(const ParticleInitializationSettings& settings) {
    validate_settings(settings);

    std::vector<ParticleGpu> particles;
    particles.reserve(settings.particle_count);

    Xorshift32 random{settings.seed};
    const float inner_radius_squared = settings.inner_radius * settings.inner_radius;
    const float outer_radius_squared = settings.outer_radius * settings.outer_radius;

    for (std::size_t index = 0; index < settings.particle_count; ++index) {
        const float angle = 2.0F * std::numbers::pi_v<float> * random.next_unit_float();
        const float radius =
            std::sqrt(inner_radius_squared +
                      (random.next_unit_float() * (outer_radius_squared - inner_radius_squared)));
        const float height = settings.half_thickness * ((2.0F * random.next_unit_float()) - 1.0F);
        const float sine = std::sin(angle);
        const float cosine = std::cos(angle);
        const float lifetime = std::lerp(settings.minimum_lifetime, settings.maximum_lifetime,
                                         random.next_unit_float());
        const float age = lifetime * random.next_unit_float();

        const glm::vec3 position{radius * cosine, height, radius * sine};
        const glm::vec3 velocity = settings.initial_orbital_speed * glm::vec3{-sine, 0.0F, cosine};

        particles.push_back(ParticleGpu{
            .position_age = glm::vec4{position, age},
            .velocity_lifetime = glm::vec4{velocity, lifetime},
            .random_state = glm::uvec4{random.state(), 0U, 0U, 0U},
        });
    }

    return particles;
}
} // namespace gps

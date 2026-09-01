#include "simulation/particle_settings.hpp"

#include <cmath>
#include <stdexcept>

namespace gps {
void validate_particle_settings(const ParticleSettings& settings) {
    if (settings.particle_count == 0) {
        throw std::invalid_argument{"Particle count must be greater than zero."};
    }
    if (!std::isfinite(settings.emitter_inner_radius) || settings.emitter_inner_radius <= 0.0F) {
        throw std::invalid_argument{"Emitter inner radius must be finite and greater than zero."};
    }
    if (!std::isfinite(settings.emitter_outer_radius) ||
        settings.emitter_outer_radius < settings.emitter_inner_radius) {
        throw std::invalid_argument{
            "Emitter outer radius must be finite and at least the inner radius."};
    }
    if (!std::isfinite(settings.emitter_half_thickness) || settings.emitter_half_thickness < 0.0F) {
        throw std::invalid_argument{"Emitter half-thickness must be finite and non-negative."};
    }
    if (!std::isfinite(settings.minimum_lifetime) || settings.minimum_lifetime <= 0.0F) {
        throw std::invalid_argument{
            "Minimum particle lifetime must be finite and greater than zero."};
    }
    if (!std::isfinite(settings.maximum_lifetime) ||
        settings.maximum_lifetime < settings.minimum_lifetime) {
        throw std::invalid_argument{
            "Maximum particle lifetime must be finite and at least the minimum lifetime."};
    }
    if (!std::isfinite(settings.orbital_speed)) {
        throw std::invalid_argument{"Orbital speed must be finite."};
    }
    if (!std::isfinite(settings.velocity_jitter) || settings.velocity_jitter < 0.0F) {
        throw std::invalid_argument{"Velocity jitter must be finite and non-negative."};
    }
    if (!std::isfinite(settings.escape_radius) ||
        settings.escape_radius <= settings.emitter_outer_radius) {
        throw std::invalid_argument{
            "Escape radius must be finite and greater than the emitter outer radius."};
    }
    if (!std::isfinite(settings.attraction_strength) || settings.attraction_strength <= 0.0F) {
        throw std::invalid_argument{"Attraction strength must be finite and greater than zero."};
    }
    if (!std::isfinite(settings.softening) || settings.softening <= 0.0F) {
        throw std::invalid_argument{"Softening must be finite and greater than zero."};
    }
    if (!std::isfinite(settings.swirl_strength)) {
        throw std::invalid_argument{"Swirl strength must be finite."};
    }
    if (!std::isfinite(settings.drag) || settings.drag < 0.0F) {
        throw std::invalid_argument{"Drag must be finite and non-negative."};
    }
    if (!std::isfinite(settings.core_radius) || settings.core_radius <= 0.0F ||
        settings.core_radius >= settings.emitter_inner_radius) {
        throw std::invalid_argument{
            "Core radius must be finite, positive, and smaller than the emitter inner radius."};
    }
}
} // namespace gps

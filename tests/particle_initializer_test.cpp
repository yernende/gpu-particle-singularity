#include "simulation/particle_initializer.hpp"

#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
[[nodiscard]] bool particles_are_equal(const gps::ParticleGpu& left,
                                       const gps::ParticleGpu& right) {
    return left.position_age == right.position_age &&
           left.velocity_lifetime == right.velocity_lifetime &&
           left.random_state == right.random_state;
}
} // namespace

TEST_CASE("particle initialization is deterministic for a seed") {
    gps::ParticleInitializationSettings settings{};
    settings.particle_count = 64;
    settings.seed = 0x1234ABCDU;

    const auto first = gps::generate_particles(settings);
    const auto second = gps::generate_particles(settings);

    REQUIRE(first.size() == settings.particle_count);
    REQUIRE(second.size() == settings.particle_count);
    CHECK(std::ranges::equal(first, second, particles_are_equal));
}

TEST_CASE("changing the seed changes the initialized particles") {
    gps::ParticleInitializationSettings settings{};
    settings.particle_count = 64;
    settings.seed = 100U;
    const auto first = gps::generate_particles(settings);

    settings.seed = 101U;
    const auto second = gps::generate_particles(settings);

    CHECK_FALSE(std::ranges::equal(first, second, particles_are_equal));
}

TEST_CASE("particles fill the requested annular volume with valid state") {
    gps::ParticleInitializationSettings settings{
        .particle_count = 4'096,
        .seed = 0U,
        .inner_radius = 1.4F,
        .outer_radius = 3.2F,
        .half_thickness = 0.2F,
        .minimum_lifetime = 6.0F,
        .maximum_lifetime = 18.0F,
        .initial_orbital_speed = 0.35F,
    };

    const auto particles = gps::generate_particles(settings);

    REQUIRE(particles.size() == settings.particle_count);
    for (const gps::ParticleGpu& particle : particles) {
        const float radius = std::hypot(particle.position_age.x, particle.position_age.z);
        const float speed = std::hypot(particle.velocity_lifetime.x, particle.velocity_lifetime.z);
        const float radial_velocity = particle.position_age.x * particle.velocity_lifetime.x +
                                      particle.position_age.z * particle.velocity_lifetime.z;

        CHECK(radius >= settings.inner_radius - 1e-5F);
        CHECK(radius <= settings.outer_radius + 1e-5F);
        CHECK(particle.position_age.y >= -settings.half_thickness);
        CHECK(particle.position_age.y <= settings.half_thickness);
        CHECK(particle.position_age.w >= 0.0F);
        CHECK(particle.position_age.w < particle.velocity_lifetime.w);
        CHECK(particle.velocity_lifetime.w >= settings.minimum_lifetime);
        CHECK(particle.velocity_lifetime.w <= settings.maximum_lifetime);
        CHECK(std::isfinite(particle.position_age.x));
        CHECK(std::isfinite(particle.position_age.y));
        CHECK(std::isfinite(particle.position_age.z));
        CHECK(std::isfinite(particle.velocity_lifetime.x));
        CHECK(std::isfinite(particle.velocity_lifetime.y));
        CHECK(std::isfinite(particle.velocity_lifetime.z));
        CHECK(particle.velocity_lifetime.y == 0.0F);
        CHECK(speed == Catch::Approx(settings.initial_orbital_speed).margin(1e-5F));
        CHECK(radial_velocity == Catch::Approx(0.0F).margin(1e-5F));
        CHECK(particle.random_state.x != 0U);
        CHECK(particle.random_state.y == 0U);
        CHECK(particle.random_state.z == 0U);
        CHECK(particle.random_state.w == 0U);
    }
}

TEST_CASE("particle initialization generates exactly the requested count") {
    gps::ParticleInitializationSettings settings{};
    settings.particle_count = 0;
    CHECK(gps::generate_particles(settings).empty());

    settings.particle_count = 17;
    CHECK(gps::generate_particles(settings).size() == settings.particle_count);
}

TEST_CASE("particle initialization rejects invalid ranges") {
    gps::ParticleInitializationSettings settings{};

    SECTION("inner radius is not positive") {
        settings.inner_radius = 0.0F;
        CHECK_THROWS_AS(gps::generate_particles(settings), std::invalid_argument);
    }

    SECTION("outer radius is smaller than inner radius") {
        settings.outer_radius = settings.inner_radius - 0.1F;
        CHECK_THROWS_AS(gps::generate_particles(settings), std::invalid_argument);
    }

    SECTION("half-thickness is negative") {
        settings.half_thickness = -0.1F;
        CHECK_THROWS_AS(gps::generate_particles(settings), std::invalid_argument);
    }

    SECTION("minimum lifetime is not positive") {
        settings.minimum_lifetime = 0.0F;
        CHECK_THROWS_AS(gps::generate_particles(settings), std::invalid_argument);
    }

    SECTION("maximum lifetime is smaller than minimum lifetime") {
        settings.maximum_lifetime = settings.minimum_lifetime - 0.1F;
        CHECK_THROWS_AS(gps::generate_particles(settings), std::invalid_argument);
    }

    SECTION("a setting is not finite") {
        settings.initial_orbital_speed = std::numeric_limits<float>::infinity();
        CHECK_THROWS_AS(gps::generate_particles(settings), std::invalid_argument);
    }
}

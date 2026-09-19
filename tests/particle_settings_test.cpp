#include "simulation/particle_settings.hpp"

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <stdexcept>

TEST_CASE("default particle settings are valid") {
    CHECK_NOTHROW(gps::validate_particle_settings(gps::ParticleSettings{}));
}

TEST_CASE("billboard appearance rejects invalid GPU inputs") {
    const std::array invalid_sizes{0.0F, -0.1F, std::numeric_limits<float>::infinity(),
                                   std::numeric_limits<float>::quiet_NaN()};
    for (const float size : invalid_sizes) {
        gps::ParticleSettings settings{};
        settings.billboard_base_size = size;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    const std::array invalid_channels{-0.1F, 1.1F, std::numeric_limits<float>::infinity(),
                                      std::numeric_limits<float>::quiet_NaN()};
    for (const float channel : invalid_channels) {
        for (std::size_t component = 0; component < 3; ++component) {
            gps::ParticleSettings settings{};
            settings.birth_color[component] = channel;
            CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);

            settings = gps::ParticleSettings{};
            settings.death_color[component] = channel;
            CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
        }
    }
}

TEST_CASE("billboard colors accept black and white endpoints") {
    gps::ParticleSettings settings{};
    settings.birth_color = {0.0F, 0.0F, 0.0F};
    settings.death_color = {1.0F, 1.0F, 1.0F};
    CHECK_NOTHROW(gps::validate_particle_settings(settings));
}

TEST_CASE("particle settings reject invalid ranges and forces") {
    gps::ParticleSettings settings{};

    SECTION("particle count is zero") {
        settings.particle_count = 0;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("core radius is not positive") {
        settings.core_radius = 0.0F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("emitter inner radius is not positive") {
        settings.emitter_inner_radius = 0.0F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("emitter inner radius does not clear the core margin") {
        settings.emitter_inner_radius =
            settings.core_radius + (gps::minimum_emitter_core_margin * 0.5F);
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("emitter outer radius is smaller than its inner radius") {
        settings.emitter_outer_radius = settings.emitter_inner_radius - 0.1F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("emitter half-thickness is negative") {
        settings.emitter_half_thickness = -0.1F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("minimum lifetime is not positive") {
        settings.minimum_lifetime = 0.0F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("maximum lifetime is smaller than the minimum") {
        settings.maximum_lifetime = settings.minimum_lifetime - 0.1F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("orbital speed is not finite") {
        settings.orbital_speed = std::numeric_limits<float>::infinity();
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("velocity jitter is negative") {
        settings.velocity_jitter = -0.1F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("escape radius does not surround the emitter") {
        settings.escape_radius = settings.emitter_outer_radius;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("attraction strength is not positive") {
        settings.attraction_strength = 0.0F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("softening is not positive") {
        settings.softening = 0.0F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("swirl strength is not finite") {
        settings.swirl_strength = std::numeric_limits<float>::infinity();
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("drag is negative") {
        settings.drag = -0.1F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("core radius reaches the emitter") {
        settings.core_radius = settings.emitter_inner_radius;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }

    SECTION("point size is not positive") {
        settings.point_size = 0.0F;
        CHECK_THROWS_AS(gps::validate_particle_settings(settings), std::invalid_argument);
    }
}

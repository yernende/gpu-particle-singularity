#include "simulation/fixed_step_accumulator.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <limits>
#include <stdexcept>

TEST_CASE("fixed-step accumulator consumes complete simulation steps") {
    gps::FixedStepAccumulator accumulator{};
    const double step = accumulator.step_seconds();

    CHECK(accumulator.advance(step * 0.5, false) == 0);
    CHECK(accumulator.advance(step * 0.5, false) == 1);
    CHECK(accumulator.advance(step * 2.0, false) == 2);
}

TEST_CASE("fixed-step accumulator bounds frame spikes and drops excess time") {
    gps::FixedStepAccumulator accumulator{};

    CHECK(accumulator.advance(0.25, false) == 8);
    CHECK(accumulator.advance(0.0, false) == 0);
    CHECK(accumulator.advance(std::numeric_limits<double>::infinity(), false) == 8);
}

TEST_CASE("fixed-step accumulator produces the same substeps at common frame rates") {
    const auto simulate_one_second = [](std::size_t frame_count, double frame_delta) {
        gps::FixedStepAccumulator accumulator{};
        std::size_t substep_count = 0;
        for (std::size_t frame = 0; frame < frame_count; ++frame) {
            substep_count += accumulator.advance(frame_delta, false);
        }
        return substep_count;
    };

    CHECK(simulate_one_second(60, 1.0 / 60.0) == 120);
    CHECK(simulate_one_second(30, 1.0 / 30.0) == 120);
}

TEST_CASE("fixed-step accumulator does not collect paused time") {
    gps::FixedStepAccumulator accumulator{};
    const double half_step = accumulator.step_seconds() * 0.5;

    CHECK(accumulator.advance(half_step, false) == 0);
    CHECK(accumulator.advance(10.0, true) == 0);
    CHECK(accumulator.advance(half_step, false) == 1);
}

TEST_CASE("fixed-step accumulator reset removes a partial step") {
    gps::FixedStepAccumulator accumulator{};
    const double half_step = accumulator.step_seconds() * 0.5;

    CHECK(accumulator.advance(half_step, false) == 0);
    accumulator.reset();
    CHECK(accumulator.advance(half_step, false) == 0);
}

TEST_CASE("fixed-step accumulator rejects invalid settings") {
    gps::FixedStepSettings settings{};

    SECTION("fixed step is not positive") {
        settings.step_seconds = 0.0;
        CHECK_THROWS_AS(gps::FixedStepAccumulator{settings}, std::invalid_argument);
    }

    SECTION("maximum frame delta is not finite") {
        settings.maximum_frame_delta_seconds = std::numeric_limits<double>::infinity();
        CHECK_THROWS_AS(gps::FixedStepAccumulator{settings}, std::invalid_argument);
    }

    SECTION("maximum substep count is zero") {
        settings.maximum_substeps_per_frame = 0;
        CHECK_THROWS_AS(gps::FixedStepAccumulator{settings}, std::invalid_argument);
    }
}

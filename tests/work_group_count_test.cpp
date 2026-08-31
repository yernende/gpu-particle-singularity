#include "simulation/work_group_count.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <stdexcept>

TEST_CASE("work-group count covers every requested invocation") {
    struct TestCase {
        std::size_t invocation_count;
        std::size_t expected_group_count;
    };

    constexpr std::size_t local_size = 256;
    constexpr std::array cases{
        TestCase{.invocation_count = 0, .expected_group_count = 0},
        TestCase{.invocation_count = 1, .expected_group_count = 1},
        TestCase{.invocation_count = 255, .expected_group_count = 1},
        TestCase{.invocation_count = 256, .expected_group_count = 1},
        TestCase{.invocation_count = 257, .expected_group_count = 2},
        TestCase{.invocation_count = 65'536, .expected_group_count = 256},
        TestCase{.invocation_count = 65'537, .expected_group_count = 257},
    };

    for (const auto& [invocation_count, expected_group_count] : cases) {
        CAPTURE(invocation_count);
        CHECK(gps::work_group_count(invocation_count, local_size) == expected_group_count);
    }
}

TEST_CASE("work-group count rejects a zero local size") {
    CHECK_THROWS_AS(gps::work_group_count(1, 0), std::invalid_argument);
}

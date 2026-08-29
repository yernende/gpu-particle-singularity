#include "support/app_options.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <string_view>

TEST_CASE("application options default to an interactive run") {
    const gps::AppOptions options = gps::parse_app_options({});

    CHECK(options.run_mode == gps::RunMode::interactive);
    CHECK_FALSE(options.show_help);
}

TEST_CASE("smoke mode is selected explicitly") {
    constexpr std::array arguments{std::string_view{"--smoke-test"}};
    const gps::AppOptions options = gps::parse_app_options(arguments);

    CHECK(options.run_mode == gps::RunMode::smoke_test);
}

TEST_CASE("help aliases are accepted") {
    constexpr std::array aliases{std::string_view{"--help"}, std::string_view{"-h"}};

    for (const std::string_view alias : aliases) {
        const std::array arguments{alias};
        const gps::AppOptions options = gps::parse_app_options(arguments);

        CAPTURE(alias);
        CHECK(options.show_help);
    }
}

TEST_CASE("unknown options fail with a useful diagnostic") {
    constexpr std::array arguments{std::string_view{"--not-an-option"}};

    try {
        static_cast<void>(gps::parse_app_options(arguments));
        FAIL("Expected an invalid_argument exception");
    } catch (const std::invalid_argument& error) {
        CHECK(std::string_view{error.what()} == "Unknown argument: --not-an-option");
    }
}

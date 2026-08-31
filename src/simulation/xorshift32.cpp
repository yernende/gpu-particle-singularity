#include "simulation/xorshift32.hpp"

#include <limits>

namespace gps {
namespace {
static_assert(std::numeric_limits<float>::is_iec559);
static_assert(std::numeric_limits<float>::radix == 2);
static_assert(std::numeric_limits<float>::digits == 24);

constexpr std::uint32_t RANDOM_STATE_FALLBACK = 0x9E3779B9U;

[[nodiscard]] constexpr std::uint32_t ensure_nonzero(std::uint32_t candidate) noexcept {
    return candidate == 0U ? RANDOM_STATE_FALLBACK : candidate;
}

[[nodiscard]] constexpr float to_unit_float(std::uint32_t random_bits) noexcept {
    return static_cast<float>(random_bits >> 8U) * 0x1p-24F;
}

static_assert(ensure_nonzero(0U) == RANDOM_STATE_FALLBACK);
static_assert(to_unit_float(0U) == 0.0F);
static_assert(to_unit_float(std::numeric_limits<std::uint32_t>::max()) < 1.0F);
} // namespace

Xorshift32::Xorshift32(std::uint32_t seed) noexcept : state_(ensure_nonzero(seed)) {}

float Xorshift32::next_unit_float() noexcept {
    return to_unit_float(next_uint());
}

std::uint32_t Xorshift32::state() const noexcept {
    return state_;
}

std::uint32_t Xorshift32::next_uint() noexcept {
    state_ ^= state_ << 13U;
    state_ ^= state_ >> 17U;
    state_ ^= state_ << 5U;
    return state_;
}
} // namespace gps

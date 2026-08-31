#pragma once

#include <cstdint>

namespace gps {
class Xorshift32 {
  public:
    explicit Xorshift32(std::uint32_t seed) noexcept;

    [[nodiscard]] float next_unit_float() noexcept;
    [[nodiscard]] std::uint32_t state() const noexcept;

  private:
    [[nodiscard]] std::uint32_t next_uint() noexcept;

    std::uint32_t state_;
};
} // namespace gps

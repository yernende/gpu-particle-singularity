#pragma once

#include <cstddef>

namespace gps {
struct FixedStepSettings {
    double step_seconds = 1.0 / 120.0;
    double maximum_frame_delta_seconds = 0.25;
    std::size_t maximum_substeps_per_frame = 8;
};

class FixedStepAccumulator final {
  public:
    explicit FixedStepAccumulator(FixedStepSettings settings = {});

    [[nodiscard]] std::size_t advance(double frame_delta_seconds, bool paused) noexcept;
    void reset() noexcept;

    [[nodiscard]] double step_seconds() const noexcept;

  private:
    FixedStepSettings settings_;
    double maximum_accumulated_seconds_;
    double accumulated_seconds_{0.0};
};
} // namespace gps

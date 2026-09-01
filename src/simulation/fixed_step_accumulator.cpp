#include "simulation/fixed_step_accumulator.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace gps {
FixedStepAccumulator::FixedStepAccumulator(FixedStepSettings settings)
    : settings_{settings},
      maximum_accumulated_seconds_{settings.step_seconds *
                                   static_cast<double>(settings.maximum_substeps_per_frame)} {
    if (!std::isfinite(settings_.step_seconds) || settings_.step_seconds <= 0.0) {
        throw std::invalid_argument{"Fixed simulation step must be finite and greater than zero."};
    }
    if (!std::isfinite(settings_.maximum_frame_delta_seconds) ||
        settings_.maximum_frame_delta_seconds <= 0.0) {
        throw std::invalid_argument{"Maximum frame delta must be finite and greater than zero."};
    }
    if (settings_.maximum_substeps_per_frame == 0) {
        throw std::invalid_argument{"Maximum simulation substeps must be greater than zero."};
    }
    if (!std::isfinite(maximum_accumulated_seconds_)) {
        throw std::invalid_argument{"Maximum accumulated simulation time must be finite."};
    }
}

std::size_t FixedStepAccumulator::advance(double frame_delta_seconds, bool paused) noexcept {
    if (paused || std::isnan(frame_delta_seconds)) {
        return 0;
    }

    const double bounded_frame_delta =
        std::clamp(frame_delta_seconds, 0.0, settings_.maximum_frame_delta_seconds);
    accumulated_seconds_ =
        std::min(accumulated_seconds_ + bounded_frame_delta, maximum_accumulated_seconds_);

    std::size_t substep_count = 0;
    while (substep_count < settings_.maximum_substeps_per_frame &&
           accumulated_seconds_ >= settings_.step_seconds) {
        accumulated_seconds_ -= settings_.step_seconds;
        ++substep_count;
    }

    return substep_count;
}

void FixedStepAccumulator::reset() noexcept {
    accumulated_seconds_ = 0.0;
}

double FixedStepAccumulator::step_seconds() const noexcept {
    return settings_.step_seconds;
}
} // namespace gps

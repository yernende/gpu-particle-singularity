#pragma once

#include <cstddef>
#include <stdexcept>

namespace gps {

[[nodiscard]] constexpr std::size_t work_group_count(std::size_t invocation_count,
                                                     std::size_t local_size) {
    if (local_size == 0) {
        throw std::invalid_argument{"Compute work-group local size must be greater than zero."};
    }

    return (invocation_count / local_size) +
           static_cast<std::size_t>(invocation_count % local_size != 0);
}

} // namespace gps

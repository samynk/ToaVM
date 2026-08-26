#pragma once
#include "asbc/frame.hpp"

namespace asbc {
    template<auto... Steps>
    constexpr void block(Frame& frame)
    {
        (Steps(frame), ...);
    }
}
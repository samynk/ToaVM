#pragma once
#include "asbc/frame.hpp"

namespace asbc {
    template<typename FrameType ,auto... Steps>
    constexpr void block(FrameType& frame)
    {
        (Steps(frame), ...);
    }
}
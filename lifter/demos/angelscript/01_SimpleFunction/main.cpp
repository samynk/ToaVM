#include <angelscript.h>

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>

#include "asbc/frame.hpp"
#include "asbc/execute.hpp"
#include "asbc/block.hpp"



constexpr std::int32_t square(std::int32_t value)
{
    using namespace asbc;
    using FrameType = Frame<std::int32_t>;
    FrameType frame(value);

    asbc::block<
        FrameType,
        execute<FrameType,asBC_SUSPEND>,
        execute<FrameType,asBC_MULi, 1, 0, 0>,
        execute<FrameType,asBC_CpyVtoR4, 1>,
        execute<FrameType,asBC_RET, 1>
    >(frame);

    return std::bit_cast<std::int32_t>(
        frame.valueRegister
    );
}

static_assert(square(0) == 0);
static_assert(square(7) == 49);
static_assert(square(-8) == 64);

int main()
{
    std::cout << "square(12) = " << square(12) << '\n';
}
#pragma once
#include <asbc/frame.hpp>
#include <tuple>

namespace asbc{
    template<
        typename FrameType,
        asEBCInstr Op,
        std::int32_t Arg0 = 0,
        std::int32_t Arg1 = 0,
        std::int32_t Arg2 = 0
    >
    constexpr void execute(FrameType& frame)
    {
        if constexpr (Op == asBC_SUSPEND) {
            // AngelScript uses this for line callbacks and suspension.
            // The first AOT implementation does not support suspension.
        }
        else if constexpr (Op == asBC_MULi) {
            constexpr auto destination =
                static_cast<std::size_t>(Arg0);

            constexpr auto left =
                static_cast<std::size_t>(Arg1);

            constexpr auto right =
                static_cast<std::size_t>(Arg2);
            frame.template set<destination>(
                frame.template get<left>() *
                frame.template get<right>()
            );
        }
        else if constexpr (Op == asBC_CpyVtoR4) {
            constexpr auto source =
                static_cast<std::size_t>(Arg0);

            frame.valueRegister =
                std::bit_cast<std::uint32_t>(
                    frame.template get<source>()
                );
        }
        else if constexpr (Op == asBC_RET) {
            frame.argumentWordsToPop =
                static_cast<std::uint16_t>(Arg0);

            frame.running = false;
        }
        else {
            static_assert(
                Op == asBC_SUSPEND ||
                Op == asBC_MULi ||
                Op == asBC_CpyVtoR4 ||
                Op == asBC_RET,
                "AngelScript opcode has not been implemented"
            );
        }
    }
}
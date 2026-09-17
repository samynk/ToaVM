#pragma once
#include "asbc/frame.hpp"
#include "asbc/instruction.hpp"

namespace asbc {
    template<typename FrameType ,auto... Steps>
    constexpr void block(FrameType& frame)
    {
        // RET must stop the surrounding block, including inside a loop/branch.
        (void)((frame.running && (Steps(frame), true)) && ...);
    }

    template<asEBCInstr Jump>
    constexpr bool jumpTaken(dword value)
    {
        const auto result = std::bit_cast<std::int32_t>(value);
        if constexpr (Jump == asBC_JZ) return result == 0;
        else if constexpr (Jump == asBC_JNZ) return result != 0;
        else if constexpr (Jump == asBC_JS) return result < 0;
        else if constexpr (Jump == asBC_JNS) return result >= 0;
        else if constexpr (Jump == asBC_JP) return result > 0;
        else if constexpr (Jump == asBC_JNP) return result <= 0;
        else static_assert(isConditionalJump(Jump), "Expected a conditional jump");
    }

    template<typename FrameType, asEBCInstr Jump, auto Setup>
    constexpr bool condition(FrameType& frame)
    {
        Setup(frame);
        return frame.running && jumpTaken<Jump>(frame.valueRegister);
    }

    template<typename FrameType, auto Cond, auto Body>
    constexpr void loop(FrameType& frame)
    {
        while (frame.running && Cond(frame)) {
            Body(frame);
        }
    }

    template<typename FrameType, auto Cond, auto Body>
    constexpr void doLoop(FrameType& frame)
    {
        if (!frame.running) return;
        do {
            Body(frame);
        } while (frame.running && Cond(frame));
    }

    template<typename FrameType, auto Cond, auto Taken, auto Fallthrough>
    constexpr void branch(FrameType& frame)
    {
        if (!frame.running) return;
        if (Cond(frame)) Taken(frame);
        else if (frame.running) Fallthrough(frame);
    }
}

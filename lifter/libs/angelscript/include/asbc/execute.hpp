#pragma once
#include <asbc/frame.hpp>
#include <tuple>

namespace asbc{
    template<typename ValueType,
        int16_t dest, int16_t left, int16_t right,
        typename FrameType, typename Operation>
    constexpr void binaryOperation(
        FrameType& frame,
        Operation operation)
    {
        
    }

    template<
        typename FrameType,
        asEBCInstr Op,
        std::int16_t arg0 = 0,
        std::int16_t arg1 = 0,
        std::int16_t arg2 = 0
    >
    constexpr void execute(FrameType& frame)
    {
        if constexpr (Op == asBC_SUSPEND) {
            // AngelScript uses this for line callbacks and suspension.
            // The first AOT implementation does not support suspension.
        }
        else if constexpr (Op == asBC_MULi) {
            frame.template set<arg0>(
                frame.template get<int32_t,arg1>() * frame.template get<int32_t,arg2>()
            );
        }else if constexpr (Op == asBC_ADDi) {
            frame.template set<arg0>(
                frame.template get<int32_t,arg1>() + frame.template get<int32_t,arg2>()
            );
        }else if constexpr (Op == asBC_MULf) {
            frame.template set<arg0>(
                frame.template get<float,arg1>() * frame.template get<float,arg2>()
            );
        }else if constexpr (Op == asBC_ADDf) {
            frame.template set<arg0>(
                frame.template get<float,arg1>() + frame.template get<float,arg2>()
            );
        }
        else if constexpr (Op == asBC_CpyVtoR4) {
            constexpr auto source =
                static_cast<std::size_t>(arg0);

            frame.setReturnValue(
                frame.template get<std::uint32_t, source>()
            );
        }
        else if constexpr (Op == asBC_RET) {
            frame.argumentWordsToPop =
                static_cast<std::uint16_t>(arg0);

            frame.running = false;
        }
        else {
            static_assert(
                Op == asBC_SUSPEND ||
                Op == asBC_MULi ||
                Op == asBC_ADDi ||
                Op == asBC_MULf ||
                Op == asBC_ADDf ||
                Op == asBC_CpyVtoR4 ||
                Op == asBC_RET,
                "AngelScript opcode has not been implemented"
            );
        }
    }
}
#pragma once
#include <asbc/frame.hpp>
#include <asbc/host_api.hpp>
#include <asbc/host_call.hpp>
#include <tuple>
#include <math.h>

namespace asbc{
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
            frame.template set<arg0>(frame.template get<float,arg1>() + frame.template get<float,arg2>());
        }
        else if constexpr (Op == asBC_CpyVtoR4) {
            frame.setReturnValue(
                frame.template get<dword, arg0>()
            );
        }else if constexpr (Op == asBC_CpyRtoV4) {
            frame.template set<arg0>(
                frame.valueRegister
            );
        }else if constexpr (Op == asBC_SetV4){
            constexpr dword value = joinOperandWords(arg1, arg2);
            frame.template set<arg0>(value);
        }else if constexpr (Op == asBC_PshV4) {
            frame.pushDWord(
                frame.template get<dword, arg0>()
            );
        }else if constexpr (Op == asBC_CALLSYS) {
            constexpr auto usedFunctionIndex = joinOperandWords(arg0, arg1);
            detail::callHostFunction<FrameType, usedFunctionIndex>(frame);
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
                Op == asBC_CpyRtoV4 ||
                Op == asBC_RET ||
                Op == asBC_PshV4 ||
                Op == asBC_CALLSYS,
                "AngelScript opcode has not been implemented"
            );
        }
    }
}

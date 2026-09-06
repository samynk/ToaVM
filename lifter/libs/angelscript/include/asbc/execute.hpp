#pragma once
#include <asbc/frame.hpp>
#include <asbc/host_api.hpp>
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
        }else if constexpr (Op == asBC_PshV4) {
            static_assert(arg0 != 0, "Argument 0 must be non-zero for asBC_PshV4");
            frame.pushDWord(
                frame.template get<dword, arg0>()
            );
        }else if constexpr (Op == asBC_CALLSYS) {
            const float argument =
                std::bit_cast<float>(frame.popDWord());

            constexpr auto usedFunctionIndex = (arg0 << 16) + arg1;
            
            using HostApi = typename FrameType::host_api_type;
            constexpr auto sqrtFunction = HostApi::template get<"sqrt">();
            float result = sqrtFunction(argument);

            //std::cout << "Result: " << result << std::endl; 
            frame.setReturnValue(
                std::bit_cast<dword>(result)
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
                Op == asBC_CpyRtoV4 ||
                Op == asBC_RET ||
                Op == asBC_PshV4 ||
                Op == asBC_CALLSYS,
                "AngelScript opcode has not been implemented"
            );
        }
    }
}
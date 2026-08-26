#pragma once
#include <array>
#include <cstdint>
#include <tuple>
namespace asbc{
    template<typename ReturnType,typename... Parameters>
    struct Frame {
        static constexpr std::size_t parameterCount = sizeof...(Parameters);
        constexpr explicit Frame(Parameters... parameters)
        : variables(parameters...)
        {

        }
        // Parameters
        std::tuple<Parameters...> variables{};
        // additional locals needed by the function
        std::array<std::int32_t, 256> locals{};

        // Four-byte view of AngelScript's value register.
        ReturnType valueRegister{};

        // RET metadata. It will matter once we implement nested calls.
        std::uint16_t argumentWordsToPop{};

        bool running{true};

        template<typename ValueType,std::int16_t Index>
        constexpr decltype(auto) get()
        {
            if constexpr (Index > 0) {
                return std::bit_cast<ValueType>(locals[Index - 1]);
            } else {
                return std::get<-Index>(variables);
            }
        }

        template<std::int16_t Index, typename Value>
        constexpr void set(Value&& value)
        {
            if constexpr (Index > 0) {
                std::int32_t localValue = std::bit_cast<std::int32_t>(std::forward<Value>(value));
                locals[Index - 1] = localValue;
            } else {
                std::get<-Index>(variables) = std::forward<Value>(value);
            }
        }

        constexpr void setReturnValue(uint32_t value) {
            valueRegister = std::bit_cast<ReturnType>(value);
        }
    };
}
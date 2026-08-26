#pragma once
#include <array>
#include <cstdint>
#include <tuple>
namespace asbc{
    template<typename... Parameters>
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
        std::uint32_t valueRegister{};

        // RET metadata. It will matter once we implement nested calls.
        std::uint16_t argumentWordsToPop{};

        bool running{true};

        template<std::size_t Index>
        constexpr decltype(auto) get()
        {
            if constexpr (Index < parameterCount) {
                return std::get<Index>(variables);
            } else {
                return locals[Index - parameterCount];
            }
        }

        template<std::size_t Index, typename Value>
        constexpr void set(Value&& value)
        {
            if constexpr (Index < parameterCount) {
                std::get<Index>(variables) =
                    std::forward<Value>(value);
            } else {
                locals[Index - parameterCount] =
                    std::forward<Value>(value);
            }
        }
    };
}
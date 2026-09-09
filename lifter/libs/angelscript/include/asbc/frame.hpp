#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>
namespace asbc{
    using dword = std::uint32_t;
    using index = std::int16_t;

    template<auto const& Module, typename HostApi>
    struct execution_environment {
        static constexpr auto const& module = Module;
        using host_api_type = HostApi;
    };

    struct no_host_api {};

    struct standalone_environment {
        using host_api_type = no_host_api;
        static constexpr bool has_module_metadata = false;
    };

    template<typename ExecEnv, typename ReturnType,typename... Parameters>
    struct Frame {
        using environment_type = ExecEnv;
        using host_api_type = typename ExecEnv::host_api_type;
        static constexpr std::size_t parameterCount = sizeof...(Parameters);

        constexpr explicit Frame(Parameters... parameters)
        : variables(parameters...)
        {

        }
        

        // Parameters
        std::tuple<Parameters...> variables{};
        // additional locals needed by the function
        std::array<dword, 256> locals{};

        // Four-byte view of AngelScript's value register.
        dword valueRegister{};

        // RET metadata. It will matter once we implement nested calls.
        index argumentWordsToPop{};

        bool running{true};
        static constexpr index operandStackCapacity = 256;

        std::array<dword, operandStackCapacity> operandStack{};
        index stackPointer = operandStackCapacity;



        template<typename ValueType,index Index>
        constexpr decltype(auto) get()
        {
            if constexpr (Index > 0) {
                return std::bit_cast<ValueType>(locals[Index - 1]);
            } else {
                return std::bit_cast<ValueType>(std::get<-Index>(variables));
            }
        }

        template<index Index, typename Value>
        constexpr void set(Value&& value)
        {
            if constexpr (Index > 0) {
                dword localValue = std::bit_cast<dword>(std::forward<Value>(value));
                locals[Index - 1] = localValue;
            } else {
                using Parameter = std::remove_cvref_t<decltype(std::get<-Index>(variables))>;
                std::get<-Index>(variables) = std::bit_cast<Parameter>(value);
            }
        }

        constexpr void setReturnValue(dword value) {
            valueRegister = value;
        }

        constexpr ReturnType getReturnValue() const {
            return std::bit_cast<ReturnType>(valueRegister);
        
        }
        // Stack implementation

        constexpr void pushDWord(dword value)
        {
            if (stackPointer == 0) {
                throw "AngelScript operand stack overflow";
            }

            operandStack[--stackPointer] = value;
        }

        constexpr dword popDWord()
        {
            if (stackPointer == operandStackCapacity) {
                throw "AngelScript operand stack underflow";
            }

            return operandStack[stackPointer++];
        }

        constexpr dword stackWord(index offset = 0) const
        {
            if (stackPointer + offset >= operandStackCapacity) {
                throw "Invalid AngelScript stack access";
            }

            return operandStack[stackPointer + offset];
        }

        constexpr index stackSize() const
        {
            return operandStackCapacity - stackPointer;
        }

        constexpr void discardStackWords(index count)
        {
            if (count > stackSize()) {
                throw "AngelScript operand stack underflow";
            }
            stackPointer += count;
        }
    };
}

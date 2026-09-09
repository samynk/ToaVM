#pragma once

#include "frame.hpp"
#include "host_api.hpp"
#include "format/used_function_reader.hpp"

#include <bit>
#include <tuple>
#include <type_traits>

namespace asbc::detail {

    // Serialized token values from AngelScript 2.38.0's as_tokendef.h.
    // The current VM stores one DWORD per value. Reject wider values and
    // references until their stack/register instructions are implemented.
    template<class T>
    consteval std::uint64_t hostTypeToken()
    {
        if constexpr (std::is_same_v<T, bool>) return 67;
        else if constexpr (std::is_same_v<T, std::int32_t>) return 70;
        else if constexpr (std::is_same_v<T, std::int8_t>) return 71;
        else if constexpr (std::is_same_v<T, std::int16_t>) return 72;
        else if constexpr (std::is_same_v<T, std::uint32_t>) return 77;
        else if constexpr (std::is_same_v<T, std::uint8_t>) return 78;
        else if constexpr (std::is_same_v<T, std::uint16_t>) return 79;
        else if constexpr (std::is_same_v<T, float>) return 81;
        else if constexpr (std::is_same_v<T, void>) return 82;
        else return 0;
    }

    template<class T>
    constexpr T popHostValue(auto& frame)
    {
        const dword word = frame.popDWord();
        if constexpr (std::is_same_v<T, bool>) {
            return (word & 0xffu) != 0;
        } else if constexpr (sizeof(T) == sizeof(dword)) {
            return std::bit_cast<T>(word);
        } else {
            return std::bit_cast<T>(static_cast<std::make_unsigned_t<T>>(word));
        }
    }

    template<class T>
    constexpr dword hostReturnWord(T value)
    {
        if constexpr (sizeof(T) == sizeof(dword)) {
            return std::bit_cast<dword>(value);
        } else {
            return static_cast<dword>(value);
        }
    }

    template<class Pointer>
    struct host_function_traits;

    template<class R, class... Args>
    struct host_function_traits<R (*)(Args...)> {
        static constexpr bool supported =
            hostTypeToken<R>() != 0 && ((hostTypeToken<Args>() != 0) && ...);

        template<std::size_t N>
        static consteval bool matches(const format::simple_function_signature<N>& signature)
        {
            if constexpr (!supported || N != sizeof...(Args)) {
                return false;
            } else {
                if (signature.return_type.token != hostTypeToken<R>() ||
                    (signature.return_type.flags & ~8u) != 0) {
                    return false;
                }
                constexpr std::array<std::uint64_t, sizeof...(Args)> tokens{hostTypeToken<Args>()...};
                for (std::size_t i = 0; i < N; ++i) {
                    if (signature.parameter_types[i].token != tokens[i] ||
                        (signature.parameter_types[i].flags & ~8u) != 0 ||
                        signature.in_out_flags[i] != 0) {
                        return false;
                    }
                }
                return true;
            }
        }

        template<auto Function, class FrameType>
        static constexpr void call(FrameType& frame)
        {
            static_assert(supported,
                "CALLSYS supports one-DWORD primitive values and void returns only");
            if constexpr (supported) {
                if (static_cast<std::size_t>(frame.stackSize()) < sizeof...(Args)) {
                    throw "AngelScript operand stack underflow in CALLSYS";
                }

                // AngelScript pushes arguments right to left; argument zero is
                // on top. Braced initialization sequences every pop left to right.
                // Function-call argument evaluation order would not guarantee this.
                std::tuple<Args...> arguments{popHostValue<Args>(frame)...};
                if constexpr (std::is_void_v<R>) {
                    std::apply(Function, arguments);
                } else {
                    frame.setReturnValue(hostReturnWord(std::apply(Function, arguments)));
                }
            }
        }
    };

    template<class R, class... Args>
    struct host_function_traits<R (*)(Args...) noexcept>
        : host_function_traits<R (*)(Args...)> {};

    template<class Environment, std::size_t FunctionIndex>
    consteval auto hostFunctionName()
    {
        constexpr auto& signature =
            std::get<FunctionIndex>(Environment::module.used_functions).signature;
        constexpr auto namespaceSize = signature.name_space.size;
        constexpr auto prefixSize = namespaceSize == 0 ? 0 : namespaceSize + 2;
        fixed_string<prefixSize + signature.name.size + 1> result{};
        for (std::size_t i = 0; i < namespaceSize; ++i) {
            result.data[i] = static_cast<char>(signature.name_space.data[i]);
        }
        if constexpr (namespaceSize != 0) {
            result.data[namespaceSize] = ':';
            result.data[namespaceSize + 1] = ':';
        }
        for (std::size_t i = 0; i < signature.name.size; ++i) {
            result.data[prefixSize + i] = static_cast<char>(signature.name.data[i]);
        }
        return result;
    }

    template<class FrameType, std::size_t FunctionIndex>
    constexpr void callHostFunction(FrameType& frame)
    {
        using Environment = typename FrameType::environment_type;
        using HostApi = typename FrameType::host_api_type;
        using UsedFunctions = std::remove_cvref_t<decltype(Environment::module.used_functions)>;
        static_assert(FunctionIndex < std::tuple_size_v<UsedFunctions>,
            "CALLSYS used-function index is out of range");
        if constexpr (FunctionIndex < std::tuple_size_v<UsedFunctions>) {
            constexpr auto& used = std::get<FunctionIndex>(Environment::module.used_functions);
            static_assert(used.origin == format::used_function_origin::application &&
                used.function_type == asFUNC_SYSTEM,
                "CALLSYS must reference an application function");
            constexpr auto name = hostFunctionName<Environment, FunctionIndex>();
            constexpr auto function = HostApi::template get<name>();
            using Traits = host_function_traits<std::remove_cv_t<decltype(function)>>;
            static_assert(Traits::supported,
                "CALLSYS supports one-DWORD primitive values and void returns only");
            static_assert(Traits::matches(used.signature),
                "Registered host function does not match the ASBC signature");
            Traits::template call<function>(frame);
        }
    }
}

#pragma once

#include <cstddef>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>

class asIScriptEngine;

// The bytecode compiler and runtime must register an identical application API.
// Add future host functions, types, and properties here for both executables.
bool register_host_api(asIScriptEngine& engine);

namespace asbc {

    template<std::size_t N>
    struct fixed_string
    {
        char data[N]{};

        consteval fixed_string(const char (&text)[N])
        {
            for (std::size_t i = 0; i < N; ++i)
                data[i] = text[i];
        }

        [[nodiscard]]
        constexpr std::string_view view() const noexcept
        {
            return {data, N - 1};
        }
    };

    template<std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;


    template<fixed_string Name, auto Function>
    struct bind
    {
        static_assert(
            std::is_pointer_v<decltype(Function)> &&
            std::is_function_v<
                std::remove_pointer_t<decltype(Function)>>,
            "bind requires a free or static function pointer"
        );

        inline static constexpr auto name = Name;
        inline static constexpr auto function = Function;

        using pointer_type = decltype(Function);
        using signature_type =
            std::remove_pointer_t<pointer_type>;
    };

    namespace detail {

        template<fixed_string Name,
                typename First,
                typename... Rest>
        consteval auto findFunction()
        {
            if constexpr (Name.view() == First::name.view()) {
                return First::function;
            }
            else if constexpr (sizeof...(Rest) != 0) {
                return findFunction<Name, Rest...>();
            }
        }
    } // namespace detail

    
    template<typename... Bindings>
    struct host_api
    {
        template<fixed_string Name>
        inline static constexpr std::size_t matchCount =
            (std::size_t{0} + ... +
            static_cast<std::size_t>(
                Name.view() == Bindings::name.view()));

        template<fixed_string Name>
        inline static constexpr bool contains =
            matchCount<Name> != 0;

        template<fixed_string Name>
        static consteval auto get()
        {
            static_assert(
                matchCount<Name> != 0,
                "Host function is not registered");

            static_assert(
                matchCount<Name> == 1,
                "Host function name is registered more than once");

            if constexpr (matchCount<Name> == 1)
                return detail::findFunction<Name, Bindings...>();
        }

        template<fixed_string Name, typename... Args>
        static constexpr decltype(auto)
        invoke(Args&&... args)
            noexcept(noexcept(
                std::invoke(
                    get<Name>(),
                    std::forward<Args>(args)...)))
        {
            return std::invoke(
                get<Name>(),
                std::forward<Args>(args)...);
        }

        template<fixed_string Name, typename Signature>
        static consteval auto resolve()
        {
            static_assert(
                std::is_function_v<Signature>,
                "Signature must be a function type");

            static_assert(
                std::is_same_v<
                    decltype(get<Name>()),
                    std::add_pointer_t<Signature>>,
                "Registered host function has the wrong signature");

            return get<Name>();
        }

        // Useful when a decoded ASBC name is currently a string_view.
        template<typename Visitor>
        static constexpr bool visit(
            std::string_view name,
            Visitor&& visitor)
        {
            bool found = false;

            ([&] {
                if (!found && name == Bindings::name.view()) {
                    std::invoke(
                        visitor,
                        std::type_identity<Bindings>{});
                    found = true;
                }
            }(), ...);

            return found;
        }
    };
} // namespace asbc
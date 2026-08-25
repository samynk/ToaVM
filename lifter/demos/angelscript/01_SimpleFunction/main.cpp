#include <angelscript.h>

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>

struct VM {
    // Parameters, locals and compiler-generated temporaries all live here.
    std::array<std::int32_t, 256> variables{};

    // Four-byte view of AngelScript's value register.
    std::uint32_t valueRegister{};

    // RET metadata. It will matter once we implement nested calls.
    std::uint16_t argumentWordsToPop{};

    bool running{true};
};

template<
    asEBCInstr Op,
    std::int32_t Arg0 = 0,
    std::int32_t Arg1 = 0,
    std::int32_t Arg2 = 0
>
constexpr void execute(VM& vm)
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

        vm.variables[destination] =
            vm.variables[left] * vm.variables[right];
    }
    else if constexpr (Op == asBC_CpyVtoR4) {
        constexpr auto source =
            static_cast<std::size_t>(Arg0);

        vm.valueRegister =
            std::bit_cast<std::uint32_t>(
                vm.variables[source]
            );
    }
    else if constexpr (Op == asBC_RET) {
        vm.argumentWordsToPop =
            static_cast<std::uint16_t>(Arg0);

        vm.running = false;
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

template<auto... Steps>
constexpr void block(VM& vm)
{
    (Steps(vm), ...);
}

constexpr std::int32_t square(std::int32_t value)
{
    VM vm;

    // AngelScript variable slot 0 contains the parameter.
    vm.variables[0] = value;

    block<
        execute<asBC_SUSPEND>,
        execute<asBC_MULi, 1, 0, 0>,
        execute<asBC_CpyVtoR4, 1>,
        execute<asBC_RET, 1>
    >(vm);

    return std::bit_cast<std::int32_t>(
        vm.valueRegister
    );
}

static_assert(square(0) == 0);
static_assert(square(7) == 49);
static_assert(square(-8) == 64);

int main()
{
    std::cout << "square(12) = " << square(12) << '\n';
}
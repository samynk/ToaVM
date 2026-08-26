#pragma once

#include "asbc/block.hpp"
#include "asbc/decode.hpp"
#include "asbc/execute.hpp"

#include <meta>
#include <vector>

namespace asbc {

template<auto const& Program>
consteval std::meta::info createBlockInfo()
{
    using namespace std::meta;
    // GCC requires static storage for a constexpr range used by template for.
    // Clang currently accepts a non-static constexpr local.
    constexpr static auto offsets =
        std::define_static_array(
            createInstructionOffsets<Program>()
        );

    std::vector<std::meta::info> steps;
    steps.reserve(offsets.size());

    template for (constexpr std::size_t offset : offsets) {
        constexpr asEBCInstr opcode =
            decodeOpcode(Program[offset]);

        constexpr Operands operands =
            decodeOperands<Program>(offset);

        constexpr std::meta::info specialization =
            substitute(
                ^^execute,
                {
                    reflect_constant(opcode),
                    reflect_constant(operands.arg0),
                    reflect_constant(operands.arg1),
                    reflect_constant(operands.arg2)
                }
            );

        steps.push_back(specialization);
    }

    return substitute(
        ^^block,
        steps
    );
}

} // namespace asbc

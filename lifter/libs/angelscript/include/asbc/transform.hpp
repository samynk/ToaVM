#pragma once

#include "asbc/block.hpp"
#include "asbc/control_flow.hpp"
#include "asbc/execute.hpp"

#include <meta>
#include <vector>

namespace asbc {
namespace detail {

template<typename FrameType, const auto& Program, std::size_t Capacity>
consteval std::meta::info lowerBlock(
    const control_flow<Capacity>& flow, std::size_t first)
{
    using namespace std::meta;
    std::vector<info> steps{^^FrameType};

    for (auto id = first; id != noControlNode; id = flow.nodes[id].next) {
        const auto& node = flow.nodes[id];
        const auto opcode = decodeOpcode(Program[node.pc]);

        if (node.kind == control_kind::instruction) {
            const auto operands = decodeOperands<Program>(node.pc);
            steps.push_back(substitute(^^execute, {
                ^^FrameType,
                reflect_constant(opcode),
                reflect_constant(operands.arg0),
                reflect_constant(operands.arg1),
                reflect_constant(operands.arg2)
            }));
            continue;
        }

        const auto setup = lowerBlock<FrameType, Program>(flow, node.setup);
        const auto predicate = substitute(^^condition, {
            ^^FrameType, reflect_constant(opcode), setup
        });
        const auto body = lowerBlock<FrameType, Program>(flow, node.body);

        if (node.kind == control_kind::branch) {
            const auto taken = lowerBlock<FrameType, Program>(flow, node.taken);
            steps.push_back(substitute(^^branch, {^^FrameType, predicate, taken, body}));
        } else {
            const auto construct = node.kind == control_kind::loop ? ^^loop : ^^doLoop;
            steps.push_back(substitute(construct, {^^FrameType, predicate, body}));
        }
    }

    return substitute(^^block, steps);
}

} // namespace detail

template<typename FrameType, const auto& Program>
consteval std::meta::info createBlockInfo()
{
    constexpr auto flow = createControlFlow<Program>();
    return detail::lowerBlock<FrameType, Program>(flow, flow.entry);
}

} // namespace asbc

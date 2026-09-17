#pragma once

#include "asbc/instruction.hpp"

#include <array>
#include <limits>

namespace asbc {

inline constexpr std::size_t noControlNode = std::numeric_limits<std::size_t>::max();

enum class control_kind { instruction, branch, loop, do_loop };

// Each block is a linked list of nodes. Child blocks describe native C++
// constructs; the completed plan has no dynamic storage and no runtime PC.
struct control_node {
    control_kind kind = control_kind::instruction;
    std::size_t pc{}; // instruction, or conditional jump supplying the predicate
    std::size_t setup = noControlNode;
    std::size_t body = noControlNode; // fallthrough arm for a branch
    std::size_t taken = noControlNode;
    std::size_t next = noControlNode;
};

template<std::size_t Capacity>
struct control_flow {
    std::array<control_node, Capacity> nodes{};
    std::size_t size{};
    std::size_t entry = noControlNode;
};

namespace detail {
    template<auto const& Program>
    class control_flow_builder {
        std::vector<std::size_t> offsets_ = instructionOffsets(Program);
        std::array<std::size_t, Program.size()> targets_{};
        control_flow<Program.size()> result_{};

        constexpr asEBCInstr opcode(std::size_t i) const
        {
            return decodeOpcode(Program[offsets_[i]]);
        }

        constexpr std::size_t add(control_kind kind, std::size_t i)
        {
            if (result_.size == result_.nodes.size()) {
                throw "AngelScript control-flow plan capacity exceeded";
            }
            const auto id = result_.size++;
            result_.nodes[id] = {.kind = kind, .pc = offsets_[i]};
            return id;
        }

        // AngelScript lays out a pre-test loop as:
        // JMP condition; body; condition; conditional-jump body.
        // The setup is straight-line; compound conditions need further lowering.
        constexpr std::size_t loopLatch(
            std::size_t condition, std::size_t end, std::size_t body) const
        {
            for (auto i = condition; i < end; ++i) {
                if (isConditionalJump(opcode(i)) && targets_[i] == body) return i;
                if (isJump(opcode(i)) || opcode(i) == asBC_RET) break;
            }
            return noControlNode;
        }

        // AngelScript can share the final RET: an early return copies its
        // value to the register and jumps to that common epilogue.
        constexpr std::size_t returnInstruction(std::size_t i) const
        {
            while (i < offsets_.size() && opcode(i) == asBC_SUSPEND) ++i;
            return i < offsets_.size() && opcode(i) == asBC_RET ? i : noControlNode;
        }

        constexpr std::size_t region(std::size_t begin, std::size_t end)
        {
            auto head = noControlNode;
            auto tail = noControlNode;
            auto append = [&](std::size_t id) {
                if (head == noControlNode) head = id;
                else result_.nodes[tail].next = id;
                tail = id;
            };

            for (auto i = begin; i < end;) {
                if (opcode(i) == asBC_JMP) {
                    const auto target = targets_[i];
                    const auto ret = returnInstruction(target);
                    if (ret != noControlNode) {
                        append(add(control_kind::instruction, ret));
                        break;
                    }
                    if (target <= i || target > end) {
                        throw "Unsupported AngelScript control flow: jump leaves a structured region";
                    }
                    const auto latch = loopLatch(target, end, i + 1);
                    if (latch != noControlNode) {
                        const auto id = add(control_kind::loop, latch);
                        result_.nodes[id].setup = region(target, latch);
                        result_.nodes[id].body = region(i + 1, target);
                        append(id);
                        i = latch + 1;
                    } else {
                        i = target; // ordinary forward jump skips unreachable code
                    }
                    continue;
                }

                // A backward conditional edge to this entry is a do/while.
                // Choose the outermost latch so nested regions remain intact.
                auto latch = noControlNode;
                for (auto j = i; j < end; ++j) {
                    if (isConditionalJump(opcode(j)) && targets_[j] == i) latch = j;
                }
                if (latch != noControlNode) {
                    const auto id = add(control_kind::do_loop, latch);
                    result_.nodes[id].body = region(i, latch);
                    append(id);
                    i = latch + 1;
                    continue;
                }

                if (isConditionalJump(opcode(i))) {
                    const auto target = targets_[i];
                    if (target <= i || target > end) {
                        throw "Unsupported AngelScript control flow: jump leaves a structured region";
                    }
                    const auto id = add(control_kind::branch, i);
                    // if/else: the fallthrough arm ends in a jump over the
                    // taken arm. Without it this is an if with an empty arm.
                    if (target > i + 1 && opcode(target - 1) == asBC_JMP &&
                        targets_[target - 1] > target &&
                        returnInstruction(targets_[target - 1]) == noControlNode) {
                        const auto join = targets_[target - 1];
                        if (join > end) {
                            throw "Unsupported AngelScript control flow: branch leaves a structured region";
                        }
                        result_.nodes[id].body = region(i + 1, target - 1);
                        result_.nodes[id].taken = region(target, join);
                        i = join;
                    } else {
                        result_.nodes[id].body = region(i + 1, target);
                        i = target;
                    }
                    append(id);
                    continue;
                }

                append(add(control_kind::instruction, i));
                ++i;
            }
            return head;
        }

    public:
        constexpr auto build()
        {
            std::array<std::size_t, Program.size()> instructionAt{};
            instructionAt.fill(noControlNode);
            for (std::size_t i = 0; i < offsets_.size(); ++i) {
                instructionAt[offsets_[i]] = i;
            }
            for (std::size_t i = 0; i < offsets_.size(); ++i) {
                if (!isJump(opcode(i))) continue;
                const auto target = jumpTarget(Program, offsets_[i]);
                if (instructionAt[target] == noControlNode) {
                    throw "AngelScript jump target is not an instruction boundary";
                }
                targets_[i] = instructionAt[target];
            }
            result_.entry = region(0, offsets_.size());
            return result_;
        }
    };
}

template<auto const& Program>
consteval auto createControlFlow()
{
    return detail::control_flow_builder<Program>{}.build();
}

} // namespace asbc

#include "loop_fixture.hpp"
#include <asbc/block.hpp>
#include <asbc/control_flow.hpp>
#include <asbc/execute.hpp>

#ifdef TOAVM_TEST_REFLECTION
#include <asbc/function.hpp>
#endif

#include <cstdlib>
#include <iostream>
#include <limits>

template<auto const& Program>
inline constexpr auto flow = asbc::createControlFlow<Program>();

// A C++20 lowering of the SAME production control-flow plan. This lets the
// parser and actual loop/branch/execute templates run on older compilers.
// TOAVM_TEST_REFLECTION instead calls the production reflection transformer.
template<auto const& Program, class Frame, std::size_t Node>
constexpr void runBlock(Frame& frame);

template<auto const& Program, class Frame, std::size_t Node>
constexpr bool testCondition(Frame& frame) {
    constexpr auto node = flow<Program>.nodes[Node];
    constexpr auto op = asbc::decodeOpcode(Program[node.pc]);
    return asbc::condition<Frame, op, runBlock<Program, Frame, node.setup>>(frame);
}

template<auto const& Program, class Frame, std::size_t Node>
constexpr void runNode(Frame& frame) {
    constexpr auto node = flow<Program>.nodes[Node];
    if constexpr (node.kind == asbc::control_kind::instruction) {
        constexpr auto op = asbc::decodeOpcode(Program[node.pc]);
        constexpr auto args = asbc::decodeOperands<Program>(node.pc);
        asbc::execute<Frame, op, args.arg0, args.arg1, args.arg2>(frame);
    } else if constexpr (node.kind == asbc::control_kind::branch) {
        asbc::branch<Frame, testCondition<Program, Frame, Node>,
            runBlock<Program, Frame, node.taken>, runBlock<Program, Frame, node.body>>(frame);
    } else if constexpr (node.kind == asbc::control_kind::loop) {
        asbc::loop<Frame, testCondition<Program, Frame, Node>,
            runBlock<Program, Frame, node.body>>(frame);
    } else {
        asbc::doLoop<Frame, testCondition<Program, Frame, Node>,
            runBlock<Program, Frame, node.body>>(frame);
    }
}

template<auto const& Program, class Frame, std::size_t Node>
constexpr void runBlock(Frame& frame) {
    if constexpr (Node != asbc::noControlNode) {
        asbc::block<Frame, runNode<Program, Frame, Node>,
            runBlock<Program, Frame, flow<Program>.nodes[Node].next>>(frame);
    }
}

template<auto const& Program, class... Args>
constexpr std::int32_t runProgram(Args... args) {
#ifdef TOAVM_TEST_REFLECTION
    return asbc::invoke<asbc::standalone_environment, Program, std::int32_t, Args...>(args...);
#else
    using Frame = asbc::Frame<asbc::standalone_environment, std::int32_t, Args...>;
    Frame frame{args...};
    runBlock<Program, Frame, flow<Program>.entry>(frame);
    return frame.getReturnValue();
#endif
}

template<auto const& Bytes, std::size_t Function, class... Args>
constexpr std::int32_t run(Args... args) {
    return runProgram<asbc::format::decodedFunctionByteCode<Bytes, Function>>(args...);
}

// The presentation bytecode, with the signed -11 encoded in its original DWORD.
inline constexpr std::array<std::uint32_t, 22> presentation{
    asBC_SUSPEND,
    asbc::format::packOpcodeWord(asBC_SetV4, 1), 0,
    asBC_SUSPEND,
    asbc::format::packOpcodeWord(asBC_SetV4, 3), 0,
    asBC_JMP, 6,
    asBC_SUSPEND, asBC_SUSPEND,
    asbc::format::packOpcodeWord(asBC_ADDi, 1), asbc::format::packOperandWord(1, 3),
    asBC_SUSPEND,
    asbc::format::packOpcodeWord(asBC_IncVi, 3),
    asBC_SUSPEND,
    asbc::format::packOpcodeWord(asBC_CMPIi, 3), 10,
    asBC_JS, 0xfffffff5u,
    asBC_SUSPEND,
    asbc::format::packOpcodeWord(asBC_CpyVtoR4, 1),
    asBC_RET
};
static_assert(asbc::jumpTarget(presentation, 6) == 14);
static_assert(asbc::jumpTarget(presentation, 17) == 8);
static_assert(runProgram<presentation>() == 45);
static_assert([] {
    std::size_t loops = 0;
    for (std::size_t i = 0; i < flow<presentation>.size; ++i)
        loops += flow<presentation>.nodes[i].kind == asbc::control_kind::loop;
    return loops == 1;
}());

template<auto const& Bytes>
consteval bool roundTrip() {
    return asbc::format::decodedFunctionByteCode<Bytes, 0> == engine_bytecode_0 &&
        asbc::format::decodedFunctionByteCode<Bytes, 1> == engine_bytecode_1 &&
        asbc::format::decodedFunctionByteCode<Bytes, 2> == engine_bytecode_2 &&
        asbc::format::decodedFunctionByteCode<Bytes, 3> == engine_bytecode_3 &&
        asbc::format::decodedFunctionByteCode<Bytes, 4> == engine_bytecode_4 &&
        asbc::format::decodedFunctionByteCode<Bytes, 5> == engine_bytecode_5 &&
        asbc::format::decodedFunctionByteCode<Bytes, 6> == engine_bytecode_6 &&
        asbc::format::decodedFunctionByteCode<Bytes, 7> == engine_bytecode_7;
}
static_assert(roundTrip<loop_asbc>());
static_assert(roundTrip<loop_stripped_asbc>());

template<auto const& Bytes>
constexpr bool checkModule() {
    return run<Bytes, 0>() == 45 &&
        run<Bytes, 1>(std::int32_t{-3}) == 0 &&
        run<Bytes, 1>(std::int32_t{0}) == 0 &&
        run<Bytes, 1>(std::int32_t{1}) == 0 &&
        run<Bytes, 1>(std::int32_t{10}) == 45 &&
        run<Bytes, 2>(std::int32_t{-1}) == 7 &&
        run<Bytes, 2>(std::int32_t{0}) == 11 &&
        run<Bytes, 3>(std::int32_t{0}) == 0 &&
        run<Bytes, 3>(std::int32_t{4}) == 6 &&
        run<Bytes, 3>(std::int32_t{11}) == 45 &&
        run<Bytes, 4>() == 12 &&
        run<Bytes, 5>(std::int32_t{0}) == 1 &&
        run<Bytes, 5>(std::int32_t{10}) == 10 &&
        run<Bytes, 6>() == -6 &&
        run<Bytes, 7>(std::int32_t{-1}) == 2 &&
        run<Bytes, 7>(std::int32_t{0}) == 5;
}
static_assert(checkModule<loop_asbc>());
static_assert(checkModule<loop_stripped_asbc>());

constexpr bool checkComparisons() {
    using Frame = asbc::Frame<asbc::standalone_environment, std::int32_t>;
    Frame frame;
    for (std::int32_t value : {std::numeric_limits<std::int32_t>::min(), -1, 0, 1,
                              std::numeric_limits<std::int32_t>::max()}) {
        frame.set<1>(value);
        asbc::execute<Frame, asBC_CMPIi, 1, 0, 0>(frame);
        if (asbc::jumpTaken<asBC_JZ>(frame.valueRegister) != (value == 0) ||
            asbc::jumpTaken<asBC_JNZ>(frame.valueRegister) != (value != 0) ||
            asbc::jumpTaken<asBC_JS>(frame.valueRegister) != (value < 0) ||
            asbc::jumpTaken<asBC_JNS>(frame.valueRegister) != (value >= 0) ||
            asbc::jumpTaken<asBC_JP>(frame.valueRegister) != (value > 0) ||
            asbc::jumpTaken<asBC_JNP>(frame.valueRegister) != (value <= 0)) return false;
    }
    frame.set<1>(std::numeric_limits<std::int32_t>::min());
    asbc::execute<Frame, asBC_CMPIi, 1, -1, 32767>(frame); // INT_MAX
    if (!asbc::jumpTaken<asBC_JS>(frame.valueRegister)) return false;
    frame.set<1>(std::numeric_limits<std::int32_t>::max());
    asbc::execute<Frame, asBC_CMPIi, 1, 0, -32768>(frame); // INT_MIN
    return asbc::jumpTaken<asBC_JP>(frame.valueRegister);
}
static_assert(checkComparisons());

// RET also stops a plain block, even when later instructions are present.
inline constexpr std::array<std::uint32_t, 7> earlyReturn{
    asbc::format::packOpcodeWord(asBC_SetV4, 1), 7,
    asbc::format::packOpcodeWord(asBC_CpyVtoR4, 1),
    asBC_RET,
    asbc::format::packOpcodeWord(asBC_SetV4, 1), 99,
    asbc::format::packOpcodeWord(asBC_CpyVtoR4, 1)
};
static_assert(runProgram<earlyReturn>() == 7);

int main(int argc, char**) {
    if (!checkModule<loop_asbc>() || !checkModule<loop_stripped_asbc>() || !checkComparisons())
        return EXIT_FAILURE;
    // The bound depends on runtime input, so this also exercises an actual loop.
    const auto bound = static_cast<std::int32_t>(argc + 9);
    if (run<loop_asbc, 1>(bound) != bound * (bound - 1) / 2) return EXIT_FAILURE;
    std::cout << "Loop and jump checks passed (debug and stripped bytecode); loopTest = "
              << runProgram<presentation>() << '\n';
}

#include <array>
#include <meta>
#include <ranges>
#include <iostream>
#include <vector>
    
enum Op : uint8_t {
    // Constants
    ICONST_M1 = 0x02,
    ICONST_0  = 0x03,
    ICONST_1  = 0x04,
    ICONST_2  = 0x05,
    ICONST_3  = 0x06,
    ICONST_4  = 0x07,
    ICONST_5  = 0x08,
    BIPUSH    = 0x10,

    // Loads (local → stack)
    ILOAD_0   = 0x1a,
    ILOAD_1   = 0x1b,
    ILOAD_2   = 0x1c,
    ILOAD_3   = 0x1d,

    // Stores (stack → local)
    ISTORE_0  = 0x3b,
    ISTORE_1  = 0x3c,
    ISTORE_2  = 0x3d,
    ISTORE_3  = 0x3e,

    // Arithmetic
    IADD      = 0x60,
    ISUB      = 0x64,
    IMUL      = 0x68,

    // Jumps
    GOTO      = 0xa7,
    IF_ICMPLT = 0xa1,

    // Misc
    IINC      = 0x84,

    // Return
    IRETURN   = 0xac,
};

// Returns the number of operand bytes following the opcode
consteval int getOpInfo(Op op) {
    switch (op) {
        case BIPUSH:    return 1;  // 1-byte operand
        case GOTO:      return 2;  // 2-byte signed offset
        case IF_ICMPLT: return 2;  // 2-byte signed offset
        case IINC:      return 2;  // local index + increment
        default:        return 0;
    }
}

struct CPU {
    std::array<int32_t, 256> stack{};
    std::array<int32_t, 256> locals{};
    uint8_t sp{0}; 
    int32_t result{0};
    bool running{true};
};

// arg  = primary operand (bipush value, iinc local index, or jump target step index)
// arg2 = secondary operand (iinc increment value)
template<Op op, int32_t arg = 0, int32_t arg2 = 0>
void execute(CPU& cpu) {
    // Constants
    if constexpr (op == ICONST_M1) cpu.stack[cpu.sp++] = -1;
    else if constexpr (op == ICONST_0) cpu.stack[cpu.sp++] = 0;
    else if constexpr (op == ICONST_1) cpu.stack[cpu.sp++] = 1;
    else if constexpr (op == ICONST_2) cpu.stack[cpu.sp++] = 2;
    else if constexpr (op == ICONST_3) cpu.stack[cpu.sp++] = 3;
    else if constexpr (op == ICONST_4) cpu.stack[cpu.sp++] = 4;
    else if constexpr (op == ICONST_5) cpu.stack[cpu.sp++] = 5;
    else if constexpr (op == BIPUSH)   cpu.stack[cpu.sp++] = static_cast<int8_t>(arg);

    // Loads
    else if constexpr (op == ILOAD_0) cpu.stack[cpu.sp++] = cpu.locals[0];
    else if constexpr (op == ILOAD_1) cpu.stack[cpu.sp++] = cpu.locals[1];
    else if constexpr (op == ILOAD_2) cpu.stack[cpu.sp++] = cpu.locals[2];
    else if constexpr (op == ILOAD_3) cpu.stack[cpu.sp++] = cpu.locals[3];

    // Stores
    else if constexpr (op == ISTORE_0) cpu.locals[0] = cpu.stack[--cpu.sp];
    else if constexpr (op == ISTORE_1) cpu.locals[1] = cpu.stack[--cpu.sp];
    else if constexpr (op == ISTORE_2) cpu.locals[2] = cpu.stack[--cpu.sp];
    else if constexpr (op == ISTORE_3) cpu.locals[3] = cpu.stack[--cpu.sp];

    // Arithmetic
    else if constexpr (op == IADD) {
        auto a = cpu.stack[--cpu.sp];
        auto b = cpu.stack[--cpu.sp];
        cpu.stack[cpu.sp++] = a + b;
    }
    else if constexpr (op == ISUB) {
        auto a = cpu.stack[--cpu.sp];
        auto b = cpu.stack[--cpu.sp];
        cpu.stack[cpu.sp++] = b - a;
    }
    else if constexpr (op == IMUL) {
        auto a = cpu.stack[--cpu.sp];
        auto b = cpu.stack[--cpu.sp];
        cpu.stack[cpu.sp++] = a * b;
    }

    // IINC — arg = local index, arg2 = increment
    else if constexpr (op == IINC) {
        cpu.locals[arg] += arg2;
    }

    // Return
    else if constexpr (op == IRETURN) {
        cpu.result = cpu.stack[--cpu.sp];
        cpu.running = false;
    }
}

template<auto... Steps>
void block(CPU& cpu) {
    (Steps(cpu), ...);
}
// Condition: run setup steps that push operands, then compare
template<auto... Setup>
bool icmplt(CPU& cpu) {
    (Setup(cpu), ...);
    auto val2 = cpu.stack[--cpu.sp];
    auto val1 = cpu.stack[--cpu.sp];
    return val1 < val2;
}
// While loop: condition returns bool, body is a block
template<auto Cond, auto Body>
void loop(CPU& cpu) {
    while (Cond(cpu)) {
        Body(cpu);
    }
}

// Equivalent to: int sum=0; for(int i=0; i<10; ++i) sum+=i; return sum;
int main() {
    CPU cpu;
    // --- init: sum = 0, i = 0 ---
    block<
        execute<ICONST_0>, execute<ISTORE_0>,
        execute<ICONST_0>, execute<ISTORE_1>
    >(cpu);
    // --- while (i < 10) { sum += i; i++; } ---
    loop<
        icmplt<execute<ILOAD_1>, execute<BIPUSH, 10>>,
        block<
            execute<ILOAD_0>, 
            execute<ILOAD_1>, 
            execute<IADD>, 
            execute<ISTORE_0>,
            execute<IINC, 1, 1>
        >
    >(cpu);
    // --- return sum ---
    execute<ILOAD_0>(cpu);
    execute<IRETURN>(cpu);
    return cpu.result;
}

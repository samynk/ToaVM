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

// int sum=0; for(int i=0; i<10; ++i) sum+=i; return sum;
// Expected result: 45
constexpr std::array<uint8_t, 22> program {
    0x03,                   //  0: iconst_0
    0x3b,                   //  1: istore_0        sum = 0
    0x03,                   //  2: iconst_0
    0x3c,                   //  3: istore_1        i = 0
    0xa7, 0x00, 0x0a,       //  4: goto +10        -> byte 14 (condition check)
    0x1a,                   //  7: iload_0         -- loop body --
    0x1b,                   //  8: iload_1
    0x60,                   //  9: iadd
    0x3b,                   // 10: istore_0        sum += i
    0x84, 0x01, 0x01,       // 11: iinc 1, 1      i++
    0x1b,                   // 14: iload_1         -- condition --
    0x10, 0x0a,             // 15: bipush 10
    0xa1, 0xff, 0xf6,       // 17: if_icmplt -10   -> byte 7 (loop body)
    0x1a,                   // 20: iload_0
    0xac,                   // 21: ireturn
};

struct CPU {
    std::array<int32_t, 256> stack{};
    std::array<int32_t, 256> locals{};
    uint8_t sp{0};
    int32_t pc{0};        // program counter (indexes into step array)
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

    // Jumps — arg holds the target STEP INDEX (resolved at compile time)
    else if constexpr (op == GOTO) {
        cpu.pc = arg;  // overrides the pc++ from the main loop
    }
    else if constexpr (op == IF_ICMPLT) {
        // JVM: pops value2 (top), then value1; branches if value1 < value2
        auto val2 = cpu.stack[--cpu.sp];
        auto val1 = cpu.stack[--cpu.sp];
        if (val1 < val2) cpu.pc = arg;
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

// Collect the byte offset of each instruction in the program
consteval std::vector<int> createOpcodeOffsets()
{
    std::vector<int> result;
    for (int idx = 0; idx < static_cast<int>(program.size()); ++idx) {
        result.push_back(idx);
        Op op = static_cast<Op>(program[idx]);
        idx += getOpInfo(op);
    }
    return result;
}

// Reverse lookup: given a byte offset, find its step index
consteval int byteToStep(int byteOffset)
{
    auto offsets = createOpcodeOffsets();
    for (int i = 0; i < static_cast<int>(offsets.size()); ++i) {
        if (offsets[i] == byteOffset) return i;
    }
    return -1; // should never happen for valid bytecode
}

consteval auto createProgram()
{
    constexpr auto tmpl = ^^execute;

    using namespace std::meta;
    constexpr auto offsets = define_static_array(createOpcodeOffsets());

    size_t pc = 0;
    std::array<void(*)(CPU&), offsets.size()> steps{};

    template for (constexpr int offset : offsets) {
        constexpr Op op = static_cast<Op>(program[offset]);

        if constexpr (op == GOTO || op == IF_ICMPLT) {
            // Read 2-byte signed branch offset, resolve to step index
            constexpr int16_t branchOffset = static_cast<int16_t>(
                (program[offset + 1] << 8) | program[offset + 2]
            );
            constexpr int32_t targetStep = byteToStep(offset + branchOffset);

            constexpr auto spec = substitute(tmpl, {
                reflect_constant(op),
                reflect_constant(targetStep),
                reflect_constant(static_cast<int32_t>(0))
            });
            steps[pc] = [:spec:];
        }
        else if constexpr (op == IINC) {
            constexpr int32_t localIdx = program[offset + 1];
            constexpr int32_t incr = static_cast<int8_t>(program[offset + 2]);

            constexpr auto spec = substitute(tmpl, {
                reflect_constant(op),
                reflect_constant(localIdx),
                reflect_constant(incr)
            });
            steps[pc] = [:spec:];
        }
        else if constexpr (op == BIPUSH) {
            constexpr int32_t barg = program[offset + 1];

            constexpr auto spec = substitute(tmpl, {
                reflect_constant(op),
                reflect_constant(barg),
                reflect_constant(static_cast<int32_t>(0))
            });
            steps[pc] = [:spec:];
        }
        else {
            constexpr auto spec = substitute(tmpl, {
                reflect_constant(op),
                reflect_constant(static_cast<int32_t>(0)),
                reflect_constant(static_cast<int32_t>(0))
            });
            steps[pc] = [:spec:];
        }
        ++pc;
    }
    return steps;
}

int loop(){
    int sum{0};
    for(int i =0;i <10;++i){
        sum+=i;
    }
    return sum;
}

int main(void) {
    constexpr auto steps = createProgram();
    CPU cpu;

    // PC-based execution loop: jumps override cpu.pc
    while (cpu.running && cpu.pc < static_cast<int32_t>(steps.size())) {
        steps[cpu.pc++](cpu);
    }

    std::cout << "Result: " << cpu.result << "\n";
    
    std::cout << "Result2: " << loop() << "\n";
    return cpu.result;
}

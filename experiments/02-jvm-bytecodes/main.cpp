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

    // Return
    IRETURN   = 0xac,
};

consteval int getOpInfo(Op op) {
    switch (op) {
        case BIPUSH:   return 1;  // one byte operand
        default:       return 0;  // all others: no operands
    }
}

constexpr std::array<uint8_t, 9> program {
    0x08,       // iconst_5
    0x3b,       // istore_0
    0x10, 0x0d, // bipush 13
    0x3c,       // istore_1
    0x1a,       // iload_0
    0x1b,       // iload_1
    0x60,       // iadd
    0xac,       // ireturn
};

struct CPU {
    std::array<int32_t, 256> stack{};
    std::array<int32_t, 256> locals{};
    uint8_t sp{0};
    int32_t result{0};
};

template<Op op, uint8_t arg = 0>
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
        cpu.stack[cpu.sp++] = b - a;  // note: b - a, not a - b!
    }
    else if constexpr (op == IMUL) {
        auto a = cpu.stack[--cpu.sp];
        auto b = cpu.stack[--cpu.sp];
        cpu.stack[cpu.sp++] = a * b;
    }

    // Return
    else if constexpr (op == IRETURN) {
        cpu.result = cpu.stack[--cpu.sp];
    }
}

consteval std::vector<int> createOpcodeOffsets()
{
    std::vector<int> result;
    for (int idx =0;idx<program.size();++idx)
    {
        result.push_back(idx);
        Op op = static_cast<Op>(program[idx]);
        idx += getOpInfo(op);
    }
    return result;
}

consteval auto createProgram()
{
    constexpr auto tmpl = ^^execute;
    
    size_t pc = 0;
    using namespace std::meta;
    constexpr auto offsets = define_static_array(createOpcodeOffsets());
    std::array<void(*)(CPU&), offsets.size()> steps{};
    template for (constexpr int offset : offsets) {
        constexpr Op op = static_cast<Op>(program[offset]);
        constexpr uint8_t arg = getOpInfo(op)==1 ? program[offset+1] : 0;
        
        // reflect the enum value and substitute into template
        constexpr auto spec = substitute(tmpl, {reflect_constant(op),reflect_constant(arg)});
        //typename [:spec:] step;
        steps[pc] = [:spec:];
        ++pc;
    }
    return steps;
}  

int main(void){
    /*
    constexpr auto offsets = define_static_array(createOpcodeOffsets());
    for(int idx =0; idx<offsets.size();++idx){
        std::cout << "opcode " << idx ;
        std::cout << " at " << offsets[idx] << "\n";
    }
    */

    constexpr auto program = createProgram();
    CPU cpu;
    int stepNr = 0;
    for (auto step : program){
        //std::cout << "step  " << stepNr << "\n";
        step(cpu);
        stepNr++;
    }
    return cpu.stack[0];
    //std::cout << "Stack contents : " << static_cast<int>(cpu.stack[0]) << "\n";
}

#include <bit>
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

    FCONST_0  = 0x0b,
    FCONST_1  = 0x0c,
    FCONST_2  = 0x0d,
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
    FADD      = 0x62,
    ISUB      = 0x64,
    IMUL      = 0x68,

    // Conversion
    F2I       = 0x8b,

    // Return
    IRETURN   = 0xac,
};

consteval int getOpInfo(Op op) {
    switch (op) {
        case BIPUSH:   return 1;  // one byte operand
        default:       return 0;  // all others: no operands
    }
}

constexpr std::array<uint8_t, 16> program {
    0x10, 0x0d, // bipush 13
    0x3c,       // istore_1
    0x0c,       // fconst_1      stack: int 5, float 1.0
    0x0d,       // fconst_2      stack: int 5, float 1.0, float 2.0
    0x62,       // fadd          stack: int 5, float 3.0
    0x8b,       // f2i           stack: int 5, int 3

    0x1b,       // iload_1       stack: int 5, int 3, int 13
    0x60,       // iadd          stack: int 5, int 16
    0xac
};

struct VM {
    using Slot = uint32_t;
    std::array<Slot, 256> stack{};
    std::array<Slot, 256> locals{};
    uint16_t sp{0};
    int32_t result{0};
};

inline VM::Slot packInt(int32_t value) {
    return std::bit_cast<uint32_t>(value);
}

inline int32_t unpackInt(VM::Slot value) {
    return std::bit_cast<int32_t>(value);
}

inline VM::Slot packFloat(float value) {
    return std::bit_cast<uint32_t>(value);
}

inline float unpackFloat(VM::Slot value) {
    return std::bit_cast<float>(value);
}

inline void pushRaw(VM& cpu, VM::Slot value) {
    cpu.stack[cpu.sp++] = value;
}

inline VM::Slot popRaw(VM& cpu) {
    return cpu.stack[--cpu.sp];
}

inline void pushInt(VM& cpu, int32_t value) {
    pushRaw(cpu, packInt(value));
}

inline int32_t popInt(VM& cpu) {
    return unpackInt(popRaw(cpu));
}

inline void pushFloat(VM& cpu, float value) {
    pushRaw(cpu, packFloat(value));
}

inline float popFloat(VM& cpu) {
    return unpackFloat(popRaw(cpu));
}

inline void printStack(VM& cpu, int stepNr){
    //std::cout << "stack at " << stepNr <<"\n";
    for(int16_t i = 0 ; i < cpu.sp ; ++i ){
        std::cout << i << ":" <<  cpu.stack[i] << "\n";
    }
    //std::cout << "\n";
}

template<Op op, uint8_t arg = 0>
void execute(VM& cpu) {
    // Integer constants
    if constexpr (op == ICONST_M1) pushInt(cpu, -1);
    else if constexpr (op == ICONST_0) pushInt(cpu, 0);
    else if constexpr (op == ICONST_1) pushInt(cpu, 1);
    else if constexpr (op == ICONST_2) pushInt(cpu, 2);
    else if constexpr (op == ICONST_3) pushInt(cpu, 3);
    else if constexpr (op == ICONST_4) pushInt(cpu, 4);
    else if constexpr (op == ICONST_5) pushInt(cpu, 5);
    else if constexpr (op == BIPUSH)   pushInt(cpu, static_cast<int8_t>(arg));

    // Float constants
    else if constexpr (op == FCONST_0) pushFloat(cpu, 0.0f);
    else if constexpr (op == FCONST_1) pushFloat(cpu, 1.0f);
    else if constexpr (op == FCONST_2) pushFloat(cpu, 2.0f);

    // Loads
    else if constexpr (op == ILOAD_0) pushRaw(cpu, cpu.locals[0]);
    else if constexpr (op == ILOAD_1) pushRaw(cpu, cpu.locals[1]);
    else if constexpr (op == ILOAD_2) pushRaw(cpu, cpu.locals[2]);
    else if constexpr (op == ILOAD_3) pushRaw(cpu, cpu.locals[3]);

    // Stores
    else if constexpr (op == ISTORE_0) cpu.locals[0] = popRaw(cpu);
    else if constexpr (op == ISTORE_1) cpu.locals[1] = popRaw(cpu);
    else if constexpr (op == ISTORE_2) cpu.locals[2] = popRaw(cpu);
    else if constexpr (op == ISTORE_3) cpu.locals[3] = popRaw(cpu);

    // Integer arithmetic
    else if constexpr (op == IADD) {
        auto a = popInt(cpu);
        auto b = popInt(cpu);
        pushInt(cpu, b + a);
    }
    else if constexpr (op == ISUB) {
        auto a = popInt(cpu);
        auto b = popInt(cpu);
        pushInt(cpu, b - a);
    }
    else if constexpr (op == IMUL) {
        auto a = popInt(cpu);
        auto b = popInt(cpu);
        pushInt(cpu, b * a);
    }

    // Floating point arithmetic
    else if constexpr (op == FADD) {
        auto a = popFloat(cpu);
        auto b = popFloat(cpu);
        pushFloat(cpu, b + a);
    }

    // Conversion
    else if constexpr (op == F2I) {
        auto value = popFloat(cpu);
        pushInt(cpu, static_cast<int32_t>(value));
    }

    // Return
    else if constexpr (op == IRETURN) {
        cpu.result = popInt(cpu);
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
    std::array<void(*)(VM&), offsets.size()> steps{};
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

/*
int32_t executeNative() {
    int32_t local_0 = 13;
    float f_sum = 1.0f + 2.0f;
    int32_t i_val = static_cast<int32_t>(f_sum);
    return i_val + local_0; // 16
}
*/

int main(void){
    constexpr auto program = createProgram();
    VM vm;
    for (auto step : program){
        step(vm);  
    }
    return vm.result;
    // return executeNative();
}
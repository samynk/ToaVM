#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>

using frame_index = std::int16_t;
using dword = std::uint32_t;

enum asEBCInstr : std::uint8_t {
    asBC_SUSPEND = 63,
    asBC_SetV4 = 77,
    asBC_ADDi = 115,
    asBC_CpyVtoR4 = 82,
    asBC_RET = 10
};

int byteCodeSize(asEBCInstr ebc)
{
    switch (ebc) {
    case asBC_SUSPEND:
    case asBC_CpyVtoR4:
    case asBC_RET:
        return 1;

    case asBC_ADDi:
    case asBC_SetV4:
        return 2;

    default:
        throw std::runtime_error("Unsupported opcode");
    }
}

constexpr frame_index lowWord(dword value)
{
    return static_cast<frame_index>(value & 0xFFFFu);
}

constexpr frame_index highWord(dword value)
{
    return static_cast<frame_index>(value >> 16);
}

// int sum()
inline constexpr std::array<dword, 11> bytecode_4{
    0xBBA1E03Fu, // SUSPEND
    0x0001624Du, // SetV4: local 1
    0x00000002u, //         constant 2
    0x0000003Fu, // SUSPEND
    0x0003C34Du, // SetV4: local 3
    0x00000005u, //         constant 5
    0x92A8C83Fu, // SUSPEND
    0x00020073u, // ADDi: destination local 2
    0x00030001u, //        source locals 1 and 3
    0x00020052u, // CpyVtoR4: local 2 -> value register
    0x0000000Au  // RET 0
};

struct Frame{
    std::array<dword, 4> locals{};
    dword valueRegister = 0;
    // Measured in DWORDs.
    frame_index pc = 0;

    template<typename T, frame_index i>
    void set(T value){
        locals[i] = std::bit_cast<dword>(value);
    }

    template <typename T, frame_index i>
    T get() const{
        return std::bit_cast<T>(locals[i]);
    }
};

template<asEBCInstr bc,frame_index arg0, frame_index arg1, frame_index arg2>
void execute(Frame& f)
{
    if constexpr( bc == asBC_SUSPEND ){

    }else if constexpr(bc == asBC_SetV4){
        dword value = (arg1<<16 | arg2);
        f.template set<dword,arg0>(value);
    }else if constexpr(bc == asBC_ADDi){
        int32_t op1 = f.template get<int32_t,arg1>();
        int32_t op2 = f.template get<int32_t,arg2>();
        f.template set<int32_t,arg0>(op1+op2); 
    }else if constexpr(bc == asBC_CpyVtoR4){
        f.valueRegister = f.template get<dword,arg0>();
    }
}

using InstructionFn = void(*)(Frame&);
template<InstructionFn ... Instructions>
void block(Frame& frame) {
    (Instructions(frame), ...);
}


int sum(){
    auto program = block<
        execute<asBC_SUSPEND,0,0,0>,
        execute<asBC_SetV4,1,0,2>,
        execute<asBC_SUSPEND,0,0,0>,
        execute<asBC_SetV4,3,0,5>,
        execute<asBC_SUSPEND,0,0,0>,
        execute<asBC_ADDi,2,1,3>,
        execute<asBC_CpyVtoR4,2,0,0>,
        execute<asBC_RET,0,0,0>
    >;
    Frame f;
    program(f);
    return std::bit_cast<int32_t>(f.valueRegister);
}

int main()
{
    return sum();
}
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>

using index = std::int16_t;
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

constexpr index lowWord(dword value)
{
    return static_cast<index>(value & 0xFFFFu);
}

constexpr index highWord(dword value)
{
    return static_cast<index>(value >> 16);
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
    index pc = 0;

    template<typename T>
    void set(index i, T value){
        locals[i] = std::bit_cast<dword>(value);
    }

    template <typename T>
    T get(index i) const{
        return std::bit_cast<T>(locals[i]);
    }
};

std::int32_t execute(std::span<const dword> bytecode)
{
    // Simplified frame: local offsets map directly to array indices.
    // This function uses slots 1, 2 and 3; slot 0 is unused.
    Frame frame;

    while (frame.pc < bytecode.size()) {
        auto pc = frame.pc;
        const dword instruction = bytecode[pc];
        const auto opcode =
            static_cast<asEBCInstr>(instruction & 0xFFu);
        const auto size =
            static_cast<index>(byteCodeSize(opcode));

        if (size > bytecode.size() - pc)
            throw std::runtime_error("Truncated instruction");

        switch (opcode) {
        case asBC_SUSPEND:
            // Treat suspension/debugger checkpoints as no-ops here.
            break;

        case asBC_SetV4: {
            const index dst = highWord(instruction);
            frame.set(dst, bytecode[pc+1]);
            break;
        }

        case asBC_ADDi: {
            const index dst = highWord(instruction);
            const index src1 = lowWord(bytecode[pc + 1]);
            const index src2 = highWord(bytecode[pc + 1]);

            int32_t op1 = frame.get<int32_t>(src1);
            int32_t op2 = frame.get<int32_t>(src2);
            frame.set(dst, op1+op2);
            break;
        }

        case asBC_CpyVtoR4: {
            const index src = highWord(instruction);
            frame.valueRegister = frame.get<int32_t>(src);
            break;
        }

        case asBC_RET:
            // This emulator runs a top-level, parameterless function.
            if ((instruction >> 16) != 0)
                throw std::runtime_error("Only RET 0 is supported");

            return std::bit_cast<std::int32_t>(frame.valueRegister);

        default:
            throw std::runtime_error("Unsupported opcode");
        }

        frame.pc += size;
    }

    throw std::runtime_error("Function ended without RET");
}

int main()
{
    return execute(bytecode_4);
}
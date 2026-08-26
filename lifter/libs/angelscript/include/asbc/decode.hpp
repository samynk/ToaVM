#pragma once

#include <angelscript.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <iomanip>

namespace asbc {

    struct Operands {
        std::uint16_t arg0{};
        std::uint16_t arg1{};
        std::uint16_t arg2{};
    };

    constexpr asEBCInstr decodeOpcode(std::uint32_t word)
    {
        return static_cast<asEBCInstr>(
            word & 0xFFu
        );
    }

    constexpr std::int16_t signedHighWord(std::uint32_t word)
    {
        return static_cast<std::int16_t>(
            word >> 16
        );
    }

    constexpr std::int16_t signedLowWord(std::uint32_t word)
    {
        return static_cast<std::int16_t>(
            word & 0xFFFFu
        );
    }

    constexpr std::uint16_t unsignedHighWord(std::uint32_t word)
    {
        std::uint16_t highWord = static_cast<std::uint16_t>(
            word >> 16
        );
        return highWord;
    }

    constexpr std::uint16_t unsignedLowWord(std::uint32_t word)
    {
        return static_cast<std::uint16_t>(
            word & 0xFFFFu
        );
    }

    constexpr std::size_t instructionSize(asEBCInstr opcode)
    {
        switch (opcode) {
        case asBC_SUSPEND:
        case asBC_CpyVtoR4:
        case asBC_RET:
            return 1;

        case asBC_MULi:
            return 2;

        default:
            return 1;
        }
    }

    template<auto const& Program>
    consteval Operands decodeOperands(
        size_t pc =0
    )
    {
        auto opcode = decodeOpcode(Program[pc]);
        switch (opcode) {
        case asBC_SUSPEND:{
            return {};
        }
        case asBC_MULi:{
            
            return {
                .arg0 = unsignedHighWord(Program[pc]),
                .arg1 = unsignedLowWord(Program[pc + 1]),
                .arg2 = unsignedHighWord(Program[pc + 1])
            };
        }
        case asBC_CpyVtoR4:{
            return {
                .arg0 = unsignedHighWord(Program[pc])
            };
        }
        case asBC_RET:{
            return {
                .arg0 = unsignedHighWord(Program[pc])
            };
        }
        default:{
            return{};
        }
        }
    }

    template<auto const& Program>
    consteval std::vector<std::size_t> createInstructionOffsets()
    {
        std::vector<std::size_t> offsets;

        std::size_t pc = 0;

        while (pc < Program.size()) {
            offsets.push_back(pc);

            const asEBCInstr opcode =
                decodeOpcode(Program[pc]);

            pc += instructionSize(opcode);
        }

        if (pc != Program.size()) {
            throw "Instruction extends beyond bytecode array";
        }

        return offsets;
    }
}
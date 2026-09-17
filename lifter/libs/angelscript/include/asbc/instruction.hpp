#pragma once

#include <angelscript.h>

#include <cstddef>
#include <cstdint>
#include <bit>
#include <vector>
#include <span>
#include <limits>
#include <iterator>


#include <iomanip>
#include "format/byte_reader.hpp"

namespace asbc {

    struct Operands {
        std::int16_t arg0{};
        std::int16_t arg1{};
        std::int16_t arg2{};
    };

    constexpr std::uint32_t joinOperandWords(std::int16_t low, std::int16_t high)
    {
        return static_cast<std::uint16_t>(low) |
            (static_cast<std::uint32_t>(static_cast<std::uint16_t>(high)) << 16u);
    }

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
        case asBC_CpyRtoV4:
        case asBC_PshV4:
        case asBC_IncVi:
        case asBC_RET:
            return 1;

        case asBC_MULi:
        case asBC_ADDi:
        case asBC_MULf:
        case asBC_ADDf:
        case asBC_CALLSYS:
        case asBC_SetV4:
        case asBC_CMPi:
        case asBC_CMPIi:
        case asBC_JMP:
        case asBC_JZ:
        case asBC_JNZ:
        case asBC_JS:
        case asBC_JNS:
        case asBC_JP:
        case asBC_JNP:
            return 2;

        default:
            throw "Unsupported AngelScript opcode";
        }
    }

    constexpr bool isConditionalJump(asEBCInstr opcode)
    {
        return opcode == asBC_JZ || opcode == asBC_JNZ ||
            opcode == asBC_JS || opcode == asBC_JNS ||
            opcode == asBC_JP || opcode == asBC_JNP;
    }

    constexpr bool isJump(asEBCInstr opcode)
    {
        return opcode == asBC_JMP || isConditionalJump(opcode);
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
        case asBC_MULi:
        case asBC_ADDi:
        case asBC_MULf:
        case asBC_ADDf:
        case asBC_SetV4:
        case asBC_CMPi:
        case asBC_CMPIi:
        {
            
            return {
                .arg0 = signedHighWord(Program[pc]),
                .arg1 = signedLowWord(Program[pc + 1]),
                .arg2 = signedHighWord(Program[pc + 1])
            };
        }
        case asBC_CpyVtoR4:
        case asBC_CpyRtoV4:
        case asBC_PshV4:
        case asBC_IncVi:
        {
            return {
                .arg0 = signedHighWord(Program[pc])
            };
        }
        case asBC_CALLSYS:
        case asBC_JMP:
        case asBC_JZ:
        case asBC_JNZ:
        case asBC_JS:
        case asBC_JNS:
        case asBC_JP:
        case asBC_JNP:
        {
            // DWORD operand: used-function index or signed jump displacement.
            return {
                .arg0 = signedLowWord(Program[pc + 1]),
                .arg1 = signedHighWord(Program[pc + 1])
            };
        }        
        case asBC_RET:{
            return {
                .arg0 = signedHighWord(Program[pc])
            };
        }
        default:{
            throw "Unsupported AngelScript opcode";
        }
        }
    }

    constexpr std::vector<std::size_t> instructionOffsets(
        std::span<const std::uint32_t> program)
    {
        std::vector<std::size_t> offsets;

        std::size_t pc = 0;

        while (pc < program.size()) {
            offsets.push_back(pc);

            const asEBCInstr opcode =
                decodeOpcode(program[pc]);

            pc += instructionSize(opcode);
        }

        if (pc != program.size()) {
            throw "Instruction extends beyond bytecode array";
        }

        return offsets;
    }

    template<auto const& Program>
    consteval std::vector<std::size_t> createInstructionOffsets()
    {
        return instructionOffsets(Program);
    }

    // In memory, a jump is relative to the next instruction, in DWORDs.
    constexpr std::size_t jumpTarget(
        std::span<const std::uint32_t> program, std::size_t pc)
    {
        if (pc >= program.size() || !isJump(decodeOpcode(program[pc])) ||
            program.size() - pc < 2) {
            throw "Expected a complete AngelScript jump instruction";
        }
        const auto target = static_cast<std::int64_t>(pc) + 2 +
            std::bit_cast<std::int32_t>(program[pc + 1]);
        if (target < 0 || target >= static_cast<std::int64_t>(program.size())) {
            throw "AngelScript jump target is out of range";
        }
        return static_cast<std::size_t>(target);
    }
} // namespace asbc

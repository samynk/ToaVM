#include "byte_reader.hpp"

namespace asbc::format {
    constexpr bool startsWith(
        asbc::format::byte_string_view value,
        const char* prefix,
        std::size_t prefixSize)
    {
        if (value.size < prefixSize) {
            return false;
        }

        for (std::size_t i = 0; i < prefixSize; ++i) {
            if (value.data[i] !=
                static_cast<unsigned char>(prefix[i])) {
                return false;
            }
        }

        return true;
    }

    // Temporary version for your current primitive-only functions.
    // Replace this later with the real signature type decoder.
    template<class Context>
    constexpr void skipSimpleDataType(Context& context)
    {
        auto& reader = context.reader();

        const auto cacheIndex = reader.read_encoded_uint();
        if (cacheIndex != 0) {
            return;
        }

        // Internal AngelScript 2.38 token value for ttIdentifier.
        constexpr std::uint64_t identifierToken = 6;

        const auto token = reader.read_encoded_uint();
        if (token == identifierToken) {
            throw "Object data types are not supported yet";
        }

        // Type flags:
        // object handle, handle-to-const, reference, read-only.
        (void)reader.read_byte();
    }

    constexpr std::int16_t readEncodedWord(asbc::format::byte_reader& reader)
    {
        const auto value = reader.read_encoded_int64();

        if (value < std::numeric_limits<std::int16_t>::min() ||
            value > std::numeric_limits<std::int16_t>::max()) {
            throw "AngelScript WORD operand is out of range";
        }

        return static_cast<std::int16_t>(value);
    }

    struct serialized_instruction {
        asEBCInstr opcode{};
        Operands operands{};
    };

    template<auto const& Asbc, class Visitor>
    constexpr void visitFirstFunctionByteCode(Visitor visitor)
    {
        constexpr auto module = inspect_simple_module(Asbc);
        constexpr std::size_t byteCount = std::size(Asbc);

        // This restart is safe under inspect_simple_module's current
        // restrictions: there are no earlier strings or data types.
        decoding_context<byteCount> context{
            std::span<const unsigned char>{
                Asbc + module.first_function.record_offset,
                byteCount - module.first_function.record_offset
            }
        };

        auto& reader = context.reader();

        if (reader.read_byte() !=
            static_cast<std::uint8_t>('f')) {
            throw "Expected a new function record";
        }

        const auto name = context.read_string();

        skipSimpleDataType(context); // Return type

        const auto parameterCount = reader.read_encoded_uint();

        for (std::uint64_t i = 0; i < parameterCount; ++i) {
            skipSimpleDataType(context);
        }

        if (parameterCount != 0) {
            const auto inOutCount = reader.read_encoded_uint();

            if (inOutCount > parameterCount) {
                throw "Invalid in/out flag count";
            }

            for (std::uint64_t i = 0; i < inOutCount; ++i) {
                (void)reader.read_encoded_uint();
            }
        }

        const auto encodedFunctionType =
            reader.read_encoded_uint();

        const bool isTemplateFunction =
            (encodedFunctionType & 128u) != 0;

        const auto functionType =
            encodedFunctionType & ~std::uint64_t{128};

        if (functionType != asFUNC_SCRIPT) {
            throw "Expected an AngelScript script function";
        }

        if (parameterCount != 0) {
            const auto defaultArgumentCount =
                reader.read_encoded_uint();

            if (defaultArgumentCount > parameterCount) {
                throw "Invalid default-argument count";
            }

            for (std::uint64_t i = 0;
                i < defaultArgumentCount;
                ++i) {
                (void)context.read_string();
            }
        }

        // WriteTypeInfo(nullptr): this is a global function.
        if (reader.read_byte() != 0) {
            throw "Methods are not supported yet";
        }

        // Global property accessors contain an extra traits byte.
        if (startsWith(name, "get_", 4) ||
            startsWith(name, "set_", 4)) {
            (void)reader.read_byte();
        }

        (void)context.read_string(); // Namespace

        if (isTemplateFunction) {
            const auto subtypeCount =
                reader.read_encoded_uint();

            for (std::uint64_t i = 0; i < subtypeCount; ++i) {
                skipSimpleDataType(context);
            }
        }

        const auto functionBits = reader.read_byte();

        if ((functionBits & 4u) != 0) {
            throw "External function has no serialized bytecode";
        }

        const auto instructionCount =
            reader.read_encoded_uint();

        for (std::uint64_t i = 0; i < instructionCount; ++i) {
            serialized_instruction instruction{
                .opcode =
                    static_cast<asEBCInstr>(reader.read_byte())
            };

            switch (instruction.opcode) {
            case asBC_SUSPEND:
                break;

            case asBC_CpyVtoR4:
            case asBC_RET:
                instruction.operands.arg0 =
                    readEncodedWord(reader);
                break;

            case asBC_MULi:
            case asBC_ADDi:
            case asBC_MULf:
            case asBC_ADDf:
                instruction.operands = {
                    .arg0 = readEncodedWord(reader),
                    .arg1 = readEncodedWord(reader),
                    .arg2 = readEncodedWord(reader)
                };
                break;

            default:
                throw "Unsupported opcode in simple ASBC reader";
            }

            visitor(instruction);
        }
    }

    constexpr std::uint32_t packOpcodeWord(
        asEBCInstr opcode,
        std::int16_t arg0 = 0)
    {
        return static_cast<std::uint8_t>(opcode) |
            (static_cast<std::uint32_t>(
                    static_cast<std::uint16_t>(arg0)) << 16u);
    }

    constexpr std::uint32_t packOperandWord(
        std::int16_t arg1,
        std::int16_t arg2)
    {
        return static_cast<std::uint16_t>(arg1) |
            (static_cast<std::uint32_t>(
                    static_cast<std::uint16_t>(arg2)) << 16u);
    }

    template<auto const& Asbc>
    consteval std::size_t firstFunctionWordCount()
    {
        std::size_t result = 0;

        visitFirstFunctionByteCode<Asbc>(
            [&result](const serialized_instruction& instruction) {
                result += instructionSize(instruction.opcode);
            });

        return result;
    }

    template<auto const& Asbc>
    consteval auto readFirstFunctionByteCode()
    {
        constexpr std::size_t wordCount =
            firstFunctionWordCount<Asbc>();

        std::array<std::uint32_t, wordCount> result{};
        std::size_t pc = 0;

        visitFirstFunctionByteCode<Asbc>(
            [&result, &pc](const serialized_instruction& instruction) {
                result[pc++] = packOpcodeWord(
                    instruction.opcode,
                    instruction.operands.arg0);

                if (instructionSize(instruction.opcode) == 2) {
                    result[pc++] = packOperandWord(
                        instruction.operands.arg1,
                        instruction.operands.arg2);
                }
            });

        return result;
    }
}
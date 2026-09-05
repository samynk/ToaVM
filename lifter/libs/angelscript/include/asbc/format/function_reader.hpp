#pragma once

#include "module_reader.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <tuple>
#include <utility>

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

    struct simple_data_type {
        std::uint64_t token{};
        std::uint8_t flags{};

        friend constexpr bool operator==(
            const simple_data_type&,
            const simple_data_type&) = default;
    };

    struct simple_function_summary {
        std::size_t index{};
        std::size_t record_offset{};
        asbc::format::byte_string_view name{};
        asbc::format::byte_string_view name_space{};
        simple_data_type return_type{};
        std::uint64_t parameter_count{};
        std::uint64_t instruction_count{};
        std::size_t byte_code_word_count{};
    };

    template<std::size_t ParameterCount>
    struct simple_function_signature {
        asbc::format::byte_string_view name{};
        asbc::format::byte_string_view name_space{};
        simple_data_type return_type{};
        std::array<simple_data_type, ParameterCount> parameter_types{};
        std::array<std::uint64_t, ParameterCount> in_out_flags{};
    };

    template<std::size_t ParameterCount, std::size_t WordCount>
    struct simple_function {
        std::size_t index{};
        std::size_t record_offset{};
        asbc::format::byte_string_view name{};
        asbc::format::byte_string_view name_space{};
        simple_data_type return_type{};
        std::array<simple_data_type, ParameterCount> parameter_types{};
        std::array<std::uint64_t, ParameterCount> in_out_flags{};
        std::array<std::uint32_t, WordCount> byte_code{};
    };

    // AngelScript keeps its string and data-type caches alive for the whole
    // serialized module. This context must therefore be shared by all function
    // records rather than restarted at each function offset.
    template<std::size_t MaxEntries>
    class simple_function_decoding_context {
    public:
        constexpr explicit simple_function_decoding_context(
            std::span<const unsigned char> bytes)
            : context_{bytes}
        {
        }

        [[nodiscard]] constexpr asbc::format::byte_reader& reader()
        {
            return context_.reader();
        }

        [[nodiscard]] constexpr asbc::format::byte_string_view read_string()
        {
            return context_.read_string();
        }

        [[nodiscard]] constexpr simple_data_type read_data_type()
        {
            auto& byteReader = reader();
            const auto cacheIndex = byteReader.read_encoded_uint();

            if (cacheIndex != 0) {
                const auto index = cacheIndex - 1;

                if (index >= data_type_count_) {
                    throw "Invalid AngelScript data-type reference";
                }

                return data_types_[static_cast<std::size_t>(index)];
            }

            // Internal AngelScript 2.38 token value for ttIdentifier.
            constexpr std::uint64_t identifierToken = 6;

            const auto token = byteReader.read_encoded_uint();
            if (token == identifierToken) {
                throw "Object data types are not supported yet";
            }

            const simple_data_type result{
                .token = token,
                .flags = byteReader.read_byte(),
            };

            if (data_type_count_ >= data_types_.size()) {
                throw "AngelScript data-type table capacity exceeded";
            }

            data_types_[data_type_count_++] = result;
            return result;
        }

    private:
        decoding_context<MaxEntries> context_;
        std::array<simple_data_type, MaxEntries> data_types_{};
        std::size_t data_type_count_ = 0;
    };

    template<class Context>
    constexpr void skipSimpleDataType(Context& context)
    {
        (void)context.read_data_type();
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

    constexpr void storeDWordOperand(
        Operands& operands,
        std::int32_t value)
    {
        const auto bits = std::bit_cast<std::uint32_t>(value);

        operands.arg1 = std::bit_cast<std::int16_t>(
            static_cast<std::uint16_t>(bits));

        operands.arg2 = std::bit_cast<std::int16_t>(
            static_cast<std::uint16_t>(bits >> 16u));
    }

    struct serialized_instruction {
        asEBCInstr opcode{};
        Operands operands{};
    };

    struct ignore_visit_event {
        template<class... Values>
        constexpr void operator()(Values&&...) const
        {
        }
    };

    constexpr std::size_t checkedCount(
        std::uint64_t count,
        std::size_t capacity,
        const char* message)
    {
        if (count > capacity) {
            throw message;
        }

        return static_cast<std::size_t>(count);
    }

    template<
        auto const& Asbc,
        class FunctionVisitor,
        class ParameterTypeVisitor = ignore_visit_event,
        class ParameterModifierVisitor = ignore_visit_event,
        class InstructionVisitor = ignore_visit_event>
    constexpr void visitFunctions(
        FunctionVisitor functionVisitor,
        ParameterTypeVisitor parameterTypeVisitor = {},
        ParameterModifierVisitor parameterModifierVisitor = {},
        InstructionVisitor instructionVisitor = {})
    {
        constexpr std::size_t byteCount = std::size(Asbc);

        simple_function_decoding_context<byteCount> context{
            std::span<const unsigned char>{Asbc, byteCount}
        };

        const auto module = read_simple_module_header(context);
        auto& reader = context.reader();

        const auto functionCount = checkedCount(
            module.function_count,
            byteCount,
            "AngelScript function count is out of range");

        for (std::size_t functionIndex = 0;
             functionIndex < functionCount;
             ++functionIndex) {
            const std::size_t recordOffset = reader.position();
            const std::uint8_t functionTag = reader.read_byte();

            if (functionTag == static_cast<std::uint8_t>('r')) {
                (void)reader.read_encoded_uint();
                throw "References in the script-function table are not supported yet";
            }

            if (functionTag != static_cast<std::uint8_t>('f')) {
                throw "Expected a new AngelScript function record";
            }

            const auto name = context.read_string();

            if (name.equals("$dlgte")) {
                throw "The delegate factory is not supported as a script function";
            }

            const auto returnType = context.read_data_type();
            const auto parameterCount = reader.read_encoded_uint();
            const auto checkedParameterCount = checkedCount(
                parameterCount,
                byteCount,
                "AngelScript parameter count is out of range");

            for (std::size_t parameterIndex = 0;
                 parameterIndex < checkedParameterCount;
                 ++parameterIndex) {
                parameterTypeVisitor(
                    functionIndex,
                    parameterIndex,
                    context.read_data_type());
            }

            if (parameterCount != 0) {
                const auto inOutCount = reader.read_encoded_uint();

                if (inOutCount > parameterCount) {
                    throw "Invalid in/out flag count";
                }

                for (std::size_t parameterIndex = 0;
                     parameterIndex < static_cast<std::size_t>(inOutCount);
                     ++parameterIndex) {
                    parameterModifierVisitor(
                        functionIndex,
                        parameterIndex,
                        reader.read_encoded_uint());
                }
            }

            const auto encodedFunctionType = reader.read_encoded_uint();
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

            const auto nameSpace = context.read_string();

            if (isTemplateFunction) {
                const auto subtypeCount = reader.read_encoded_uint();
                const auto checkedSubtypeCount = checkedCount(
                    subtypeCount,
                    byteCount,
                    "AngelScript template subtype count is out of range");

                for (std::size_t i = 0;
                     i < checkedSubtypeCount;
                     ++i) {
                    skipSimpleDataType(context);
                }
            }

            const auto functionBits = reader.read_byte();

            if ((functionBits & 4u) != 0) {
                throw "External function has no serialized bytecode";
            }

            const auto instructionCount = reader.read_encoded_uint();
            const auto checkedInstructionCount = checkedCount(
                instructionCount,
                byteCount,
                "AngelScript instruction count is out of range");
            std::size_t wordCount = 0;

            for (std::size_t i = 0;
                 i < checkedInstructionCount;
                 ++i) {
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
                case asBC_CALLSYS:
                    storeDWordOperand(
                        instruction.operands,
                        reader.readEncodedDWord()
                    );
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

                instructionVisitor(functionIndex, instruction);
                wordCount += instructionSize(instruction.opcode);
            }

            // The remainder of WriteFunction must be consumed before the next
            // function record can be decoded.
            (void)reader.read_encoded_uint(); // variableSpace

            if ((functionBits & 8u) != 0) {
                const auto objectVariableInfoCount =
                    reader.read_encoded_uint();
                const auto checkedObjectVariableInfoCount = checkedCount(
                    objectVariableInfoCount,
                    byteCount,
                    "AngelScript object-variable info count is out of range");

                for (std::size_t i = 0;
                     i < checkedObjectVariableInfoCount;
                     ++i) {
                    (void)reader.read_encoded_uint();  // programPos
                    (void)reader.read_encoded_int64(); // variableOffset
                    (void)reader.read_encoded_uint();  // option
                }
            }

            if ((functionBits & 16u) != 0) {
                const auto tryCatchCount = reader.read_encoded_uint();
                const auto checkedTryCatchCount = checkedCount(
                    tryCatchCount,
                    byteCount,
                    "AngelScript try/catch count is out of range");

                for (std::size_t i = 0;
                     i < checkedTryCatchCount;
                     ++i) {
                    (void)reader.read_encoded_uint(); // tryPos
                    (void)reader.read_encoded_uint(); // catchPos
                    (void)reader.read_encoded_uint(); // stackSize
                }
            }

            if (!module.debug_info_stripped) {
                const auto lineNumberCount = reader.read_encoded_uint();
                const auto checkedLineNumberCount = checkedCount(
                    lineNumberCount,
                    byteCount,
                    "AngelScript line-number count is out of range");

                for (std::size_t i = 0;
                     i < checkedLineNumberCount;
                     ++i) {
                    (void)reader.read_encoded_uint();
                }

                const auto sectionEntryCount = reader.read_encoded_uint();
                const auto checkedSectionEntryCount = checkedCount(
                    sectionEntryCount,
                    byteCount,
                    "AngelScript section-entry count is out of range");

                for (std::size_t i = 0;
                     i < checkedSectionEntryCount;
                     ++i) {
                    if ((i & 1u) == 0) {
                        (void)reader.read_encoded_uint();
                    } else {
                        (void)context.read_string();
                    }
                }
            }

            // Variable types are present even when debug information was
            // stripped, and they participate in the module-wide type cache.
            const auto variableCount = reader.read_encoded_uint();
            const auto checkedVariableCount = checkedCount(
                variableCount,
                byteCount,
                "AngelScript variable count is out of range");

            for (std::size_t i = 0;
                 i < checkedVariableCount;
                 ++i) {
                if (!module.debug_info_stripped) {
                    (void)reader.read_encoded_uint(); // declaredAtProgramPos
                    (void)context.read_string();      // variable name
                }

                (void)reader.read_encoded_int64(); // stackOffset + onHeap
                skipSimpleDataType(context);
            }

            if (!module.debug_info_stripped) {
                (void)context.read_string();      // script section
                (void)reader.read_encoded_uint(); // declaredAt

                const auto parameterNameCount =
                    reader.read_encoded_uint();

                if (parameterNameCount > parameterCount) {
                    throw "Invalid parameter-name count";
                }

                for (std::uint64_t i = 0;
                     i < parameterNameCount;
                     ++i) {
                    (void)context.read_string();
                }
            }

            functionVisitor(simple_function_summary{
                .index = functionIndex,
                .record_offset = recordOffset,
                .name = name,
                .name_space = nameSpace,
                .return_type = returnType,
                .parameter_count = parameterCount,
                .instruction_count = instructionCount,
                .byte_code_word_count = wordCount,
            });
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
    consteval auto inspectFunctions()
    {
        constexpr auto module = inspect_simple_module(Asbc);
        constexpr std::size_t functionCount =
            static_cast<std::size_t>(module.function_count);

        std::array<simple_function_summary, functionCount> result{};

        visitFunctions<Asbc>(
            [&result](const simple_function_summary& function) {
                result[function.index] = function;
            });

        return result;
    }

    template<auto const& Asbc, std::size_t FunctionIndex>
    consteval simple_function_summary inspectFunction()
    {
        constexpr auto functions = inspectFunctions<Asbc>();
        static_assert(
            FunctionIndex < functions.size(),
            "AngelScript function index is out of range");
        return functions[FunctionIndex];
    }

    template<auto const& Asbc, std::size_t FunctionIndex>
    consteval auto readFunctionSignature()
    {
        constexpr auto summary = inspectFunction<Asbc, FunctionIndex>();
        constexpr std::size_t parameterCount =
            static_cast<std::size_t>(summary.parameter_count);

        simple_function_signature<parameterCount> result{
            .name = summary.name,
            .name_space = summary.name_space,
            .return_type = summary.return_type,
        };

        visitFunctions<Asbc>(
            ignore_visit_event{},
            [&result](
                std::size_t functionIndex,
                std::size_t parameterIndex,
                simple_data_type type) {
                if (functionIndex == FunctionIndex) {
                    result.parameter_types[parameterIndex] = type;
                }
            },
            [&result](
                std::size_t functionIndex,
                std::size_t parameterIndex,
                std::uint64_t modifier) {
                if (functionIndex == FunctionIndex) {
                    result.in_out_flags[parameterIndex] = modifier;
                }
            });

        return result;
    }

    template<auto const& Asbc, std::size_t FunctionIndex>
    consteval auto readFunctionByteCode()
    {
        constexpr auto summary = inspectFunction<Asbc, FunctionIndex>();
        constexpr std::size_t wordCount = summary.byte_code_word_count;

        std::array<std::uint32_t, wordCount> result{};
        std::size_t pc = 0;

        visitFunctions<Asbc>(
            ignore_visit_event{},
            ignore_visit_event{},
            ignore_visit_event{},
            [&result, &pc](
                std::size_t functionIndex,
                const serialized_instruction& instruction) {
                if (functionIndex != FunctionIndex) {
                    return;
                }

                result[pc++] = packOpcodeWord(
                    instruction.opcode,
                    instruction.operands.arg0);

                if (instructionSize(instruction.opcode) == 2) {
                    result[pc++] = packOperandWord(
                        instruction.operands.arg1,
                        instruction.operands.arg2);
                }
            });

        if (pc != wordCount) {
            throw "Decoded AngelScript bytecode size is inconsistent";
        }

        return result;
    }

    template<auto const& Asbc, std::size_t FunctionIndex>
    consteval auto readFunction()
    {
        constexpr auto summary = inspectFunction<Asbc, FunctionIndex>();
        constexpr auto signature =
            readFunctionSignature<Asbc, FunctionIndex>();
        constexpr auto byteCode =
            readFunctionByteCode<Asbc, FunctionIndex>();

        return simple_function<
            signature.parameter_types.size(),
            byteCode.size()>{
            .index = FunctionIndex,
            .record_offset = summary.record_offset,
            .name = signature.name,
            .name_space = signature.name_space,
            .return_type = signature.return_type,
            .parameter_types = signature.parameter_types,
            .in_out_flags = signature.in_out_flags,
            .byte_code = byteCode,
        };
    }

    template<auto const& Asbc, std::size_t... FunctionIndices>
    consteval auto readFunctionsImpl(
        std::index_sequence<FunctionIndices...>)
    {
        return std::tuple{
            readFunction<Asbc, FunctionIndices>()...
        };
    }

    template<auto const& Asbc>
    consteval auto readFunctions()
    {
        constexpr auto functions = inspectFunctions<Asbc>();
        return readFunctionsImpl<Asbc>(
            std::make_index_sequence<functions.size()>{});
    }

    template<auto const& Asbc>
    inline constexpr auto decodedFunctions = readFunctions<Asbc>();

    // This standalone variable-template copy has static storage and can be
    // passed directly to the existing template<auto const& Program> compiler.
    template<auto const& Asbc, std::size_t FunctionIndex>
    inline constexpr auto decodedFunctionByteCode =
        readFunctionByteCode<Asbc, FunctionIndex>();

    template<auto const& Asbc, class Visitor>
    constexpr void visitFirstFunctionByteCode(Visitor visitor)
    {
        visitFunctions<Asbc>(
            ignore_visit_event{},
            ignore_visit_event{},
            ignore_visit_event{},
            [visitor](
                std::size_t functionIndex,
                const serialized_instruction& instruction) mutable {
                if (functionIndex == 0) {
                    visitor(instruction);
                }
            });
    }

    template<auto const& Asbc>
    consteval std::size_t firstFunctionWordCount()
    {
        return inspectFunction<Asbc, 0>().byte_code_word_count;
    }

    template<auto const& Asbc>
    consteval auto readFirstFunctionByteCode()
    {
        return readFunctionByteCode<Asbc, 0>();
    }
}

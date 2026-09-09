#pragma once

#include "function_reader.hpp"

namespace asbc::format {

    struct simple_used_function_summary {
        std::size_t index{};
        used_function_origin origin{};
        simple_function_summary signature{};
    };

    // In the supported class-free module format all global functions are
    // references to records already read from scriptFunctions[].
    template<class Context, class Visitor = ignore_visit_event>
    constexpr void readGlobalFunctions(
        Context& context, std::size_t functionCount, Visitor visitor = {})
    {
        auto& reader = context.reader();
        const auto count = checkedCount(reader.read_encoded_uint(), functionCount,
            "AngelScript global-function count is out of range");
        for (std::size_t i = 0; i < count; ++i) {
            if (reader.read_byte() != 'r') {
                throw "Expected a global-function reference";
            }
            const auto index = reader.read_encoded_uint();
            if (index >= functionCount) {
                throw "Invalid AngelScript global-function reference";
            }
            visitor(i, static_cast<std::size_t>(index));
        }
    }

    template<auto const& Asbc, class Context>
    constexpr std::size_t beginUsedFunctions(Context& context)
    {
        constexpr auto byteCount = std::size(Asbc);
        const auto module = read_simple_module_header(context);
        visitScriptFunctions<byteCount>(context, module, ignore_visit_event{});
        readGlobalFunctions(context, static_cast<std::size_t>(module.function_count));

        auto& reader = context.reader();
        if (reader.read_encoded_uint() != 0) {
            throw "Imported function bindings are not supported yet";
        }
        if (reader.read_encoded_uint() != 0) {
            throw "Used object types are not supported yet";
        }

        const auto typeIdCount = checkedCount(reader.read_encoded_uint(), byteCount,
            "AngelScript used-type-id count is out of range");
        for (std::size_t i = 0; i < typeIdCount; ++i) {
            (void)context.read_data_type();
        }

        return checkedCount(reader.read_encoded_uint(), byteCount,
            "AngelScript used-function count is out of range");
    }

    template<auto const& Asbc>
    consteval std::size_t usedFunctionCount()
    {
        simple_function_decoding_context<std::size(Asbc)> context{
            std::span<const unsigned char>{Asbc, std::size(Asbc)}
        };
        return beginUsedFunctions<Asbc>(context);
    }

    template<auto const& Asbc, class FunctionVisitor,
        class ParameterTypeVisitor = ignore_visit_event,
        class ParameterModifierVisitor = ignore_visit_event>
    constexpr void visitUsedFunctions(
        FunctionVisitor functionVisitor,
        ParameterTypeVisitor parameterTypeVisitor = {},
        ParameterModifierVisitor parameterModifierVisitor = {})
    {
        constexpr auto byteCount = std::size(Asbc);
        simple_function_decoding_context<byteCount> context{
            std::span<const unsigned char>{Asbc, byteCount}
        };
        const auto count = beginUsedFunctions<Asbc>(context);
        for (std::size_t i = 0; i < count; ++i) {
            simple_used_function_summary function{
                .index = i,
                .origin = static_cast<used_function_origin>(context.reader().read_byte()),
            };
            switch (function.origin) {
            case used_function_origin::application:
            case used_function_origin::module:
            case used_function_origin::shared:
                function.signature = readSimpleFunctionSignature(context, i, byteCount,
                    parameterTypeVisitor, parameterModifierVisitor);
                break;
            case used_function_origin::null:
                break;
            default:
                throw "Invalid AngelScript used-function origin";
            }
            functionVisitor(function);
        }
    }

    template<auto const& Asbc>
    consteval auto inspectUsedFunctions()
    {
        std::array<simple_used_function_summary, usedFunctionCount<Asbc>()> result{};
        visitUsedFunctions<Asbc>([&](const auto& function) {
            result[function.index] = function;
        });
        return result;
    }

    template<auto const& Asbc, std::size_t FunctionIndex>
    consteval auto readUsedFunction()
    {
        constexpr auto functions = inspectUsedFunctions<Asbc>();
        static_assert(FunctionIndex < functions.size(),
            "AngelScript used-function index is out of range");
        constexpr auto summary = functions[FunctionIndex];
        simple_used_function<summary.signature.parameter_count> result{
            .index = FunctionIndex,
            .origin = summary.origin,
            .function_type = summary.signature.function_type,
            .signature = {
                .name = summary.signature.name,
                .name_space = summary.signature.name_space,
                .return_type = summary.signature.return_type,
            },
        };
        visitUsedFunctions<Asbc>(ignore_visit_event{},
            [&](std::size_t index, std::size_t parameter, simple_data_type type) {
                if (index == FunctionIndex) {
                    result.signature.parameter_types[parameter] = type;
                }
            },
            [&](std::size_t index, std::size_t parameter, std::uint64_t modifier) {
                if (index == FunctionIndex) {
                    result.signature.in_out_flags[parameter] = modifier;
                }
            });
        return result;
    }

    template<auto const& Asbc, std::size_t... Indices>
    consteval auto readUsedFunctionsImpl(std::index_sequence<Indices...>)
    {
        return std::tuple{readUsedFunction<Asbc, Indices>()...};
    }

    template<auto const& Asbc>
    consteval auto readUsedFunctions()
    {
        return readUsedFunctionsImpl<Asbc>(
            std::make_index_sequence<usedFunctionCount<Asbc>()>{});
    }

    template<class UsedFunctions>
    struct simple_module : simple_module_info {
        UsedFunctions used_functions{};
    };

    template<auto const& Asbc>
    consteval auto readSimpleModule()
    {
        auto usedFunctions = readUsedFunctions<Asbc>();
        return simple_module<decltype(usedFunctions)>{
            inspect_simple_module(Asbc), usedFunctions
        };
    }
}

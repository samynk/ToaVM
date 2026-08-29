#include "decoding_context.hpp"

namespace asbc::format {
    struct function_prefix {
        std::size_t record_offset;
        asbc::format::byte_string_view name;
    };

    struct simple_module_info {
        bool debug_info_stripped;
        std::uint64_t function_count;
        std::size_t function_records_offset;

        function_prefix first_function;
    };

    template<std::size_t Size>
    [[nodiscard]] consteval simple_module_info inspect_simple_module(
        const unsigned char (&bytes)[Size])
    {
        decoding_context<Size> context{
        std::span<const unsigned char>{bytes, Size}
        };
        auto& reader = context.reader();

        const bool debug_info_stripped =
            reader.read_encoded_uint() != 0;

        const auto enum_count = reader.read_encoded_uint();
        if (enum_count != 0) {
            throw "Initial ASBC reader does not support enums yet";
        }

        const auto class_count = reader.read_encoded_uint();
        if (class_count != 0) {
            throw "Initial ASBC reader does not support classes yet";
        }

        const auto funcdef_count = reader.read_encoded_uint();
        if (funcdef_count != 0) {
            throw "Initial ASBC reader does not support funcdefs yet";
        }

        // The intervening class/interface phases contain no data when
        // class_count is zero.

        const auto typedef_count = reader.read_encoded_uint();
        if (typedef_count != 0) {
            throw "Initial ASBC reader does not support typedefs yet";
        }

        const auto global_count = reader.read_encoded_uint();
        if (global_count != 0) {
            throw "Initial ASBC reader does not support globals yet";
        }

        const auto function_count = reader.read_encoded_uint();

        if (function_count == 0) {
            throw "AngelScript module contains no script functions";
        }

        const std::size_t first_function_offset = reader.position();

        const std::uint8_t function_tag = reader.read_byte();

        if (function_tag == static_cast<std::uint8_t>('r')) {
            throw "First script function is unexpectedly a reference";
        }

        if (function_tag != static_cast<std::uint8_t>('f')) {
            throw "Expected a new AngelScript function record";
        }

        const asbc::format::byte_string_view first_function_name =
            context.read_string();

        return {
            .debug_info_stripped = debug_info_stripped,
            .function_count = function_count,
            .function_records_offset = first_function_offset,
            .first_function = {
                .record_offset = first_function_offset,
                .name = first_function_name,
            },
        };
    }
}
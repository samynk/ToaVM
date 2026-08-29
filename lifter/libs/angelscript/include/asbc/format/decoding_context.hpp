   #include <array>

namespace asbc::format{

    template<std::size_t MaxStrings>
    class decoding_context {
        public:
            constexpr explicit decoding_context(
                std::span<const unsigned char> bytes)
                : reader_{bytes}
            {
            }

            [[nodiscard]] constexpr asbc::format::byte_reader& reader()
            {
                return reader_;
            }

            [[nodiscard]] constexpr asbc::format::byte_string_view read_string()
            {
                const std::uint64_t encoded = reader_.read_encoded_uint();

                // Odd value: reference to an earlier string.
                if ((encoded & 1u) != 0) {
                    const std::uint64_t index = encoded / 2u;

                    if (index >= string_count_) {
                        throw "Invalid AngelScript string reference";
                    }

                    return strings_[static_cast<std::size_t>(index)];
                }

                // Even value: length of a new string, multiplied by two.
                const std::uint64_t length = encoded / 2u;

                if (length == 0) {
                    return {};
                }

                if (string_count_ >= strings_.size()) {
                    throw "AngelScript string table capacity exceeded";
                }

                const auto string =
                    reader_.read_bytes(static_cast<std::size_t>(length));

                strings_[string_count_++] = string;
                return string;
            }

        private:
            asbc::format::byte_reader reader_;
            std::array<asbc::format::byte_string_view, MaxStrings> strings_{};
            std::size_t string_count_ = 0;
    };
}
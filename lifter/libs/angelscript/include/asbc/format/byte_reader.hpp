#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace asbc::format {
    struct byte_string_view {
        const unsigned char* data = nullptr;
        std::size_t size = 0;

        [[nodiscard]] constexpr const unsigned char* begin() const
        {
            return data;
        }

        [[nodiscard]] constexpr const unsigned char* end() const
        {
            return data + size;
        }

        template<std::size_t Size>
        [[nodiscard]] constexpr bool equals(
            const char (&text)[Size]) const
        {
            static_assert(Size > 0);

            if (size != Size - 1) {
                return false;
            }

            for (std::size_t i = 0; i < size; ++i) {
                if (data[i] != static_cast<unsigned char>(text[i])) {
                    return false;
                }
            }

            return true;
        }
    };

    class byte_reader {
    public:
        constexpr explicit byte_reader(std::span<const unsigned char> bytes)
            : bytes_{bytes}
        {
        }

        [[nodiscard]] constexpr std::size_t position() const
        {
            return position_;
        }

        [[nodiscard]] constexpr std::uint8_t read_byte()
        {
            if (position_ >= bytes_.size()) {
                throw "Unexpected end of AngelScript bytecode";
            }

            return bytes_[position_++];
        }

        [[nodiscard]] constexpr std::int64_t read_encoded_int64()
        {
            const std::uint8_t first = read_byte();

            const bool negative = (first & 0x80u) != 0;
            const std::uint8_t prefix = first & 0x7Fu;

            std::uint64_t magnitude = 0;
            unsigned payload_bytes = 0;

            if (prefix < 0x40u) {
                magnitude = prefix;
                payload_bytes = 0;
            } else if (prefix < 0x60u) {
                magnitude = prefix & 0x1Fu;
                payload_bytes = 1;
            } else if (prefix < 0x70u) {
                magnitude = prefix & 0x0Fu;
                payload_bytes = 2;
            } else if (prefix < 0x78u) {
                magnitude = prefix & 0x07u;
                payload_bytes = 3;
            } else if (prefix < 0x7Cu) {
                magnitude = prefix & 0x03u;
                payload_bytes = 4;
            } else if (prefix < 0x7Eu) {
                magnitude = prefix & 0x01u;
                payload_bytes = 5;
            } else if (prefix == 0x7Eu) {
                magnitude = 0;
                payload_bytes = 6;
            } else {
                magnitude = 0;
                payload_bytes = 8;
            }

            for (unsigned i = 0; i < payload_bytes; ++i) {
                magnitude = (magnitude << 8u) | read_byte();
            }

            constexpr auto minimum_magnitude =
                std::uint64_t{1} << 63u;

            if (negative) {
                if (magnitude > minimum_magnitude) {
                    throw "Encoded AngelScript integer is out of range";
                }

                if (magnitude == minimum_magnitude) {
                    return std::numeric_limits<std::int64_t>::min();
                }

                return -static_cast<std::int64_t>(magnitude);
            }

            if (magnitude >
                static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max())) {
                throw "Encoded AngelScript integer is out of range";
            }

            return static_cast<std::int64_t>(magnitude);
        }

        [[nodiscard]] constexpr std::uint64_t read_encoded_uint()
        {
            const std::int64_t value = read_encoded_int64();

            if (value < 0) {
                throw "Expected an unsigned AngelScript integer";
            }

            return static_cast<std::uint64_t>(value);
        }

        [[nodiscard]] constexpr std::int32_t readEncodedDWord()
        {
            const auto value = read_encoded_int64();

            if (value < std::numeric_limits<std::int32_t>::min() ||
                value > std::numeric_limits<std::int32_t>::max()) {
                throw "AngelScript DWORD operand is out of range";
            }

            return static_cast<std::int32_t>(value);
        }

        [[nodiscard]] constexpr byte_string_view read_bytes(std::size_t count)
        {
            if (count > bytes_.size() - position_) {
                throw "Unexpected end of AngelScript bytecode";
            }

            const byte_string_view result{
                .data = bytes_.data() + position_,
                .size = count,
            };

            position_ += count;
            return result;
        }

    private:
        std::span<const unsigned char> bytes_;
        std::size_t position_ = 0;
    };
}

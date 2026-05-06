// src/jbc/ClassReader.hpp or inside ClassFile.cpp for now
#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>

namespace jbc
{
    class ClassReader
    {
    public:
        explicit ClassReader(std::span<const std::uint8_t> bytes)
            : m_bytes(bytes)
        {
        }

        std::uint8_t read_u1()
        {
            ensure_available(1);
            return m_bytes[m_offset++];
        }

        std::uint16_t read_u2()
        {
            ensure_available(2);

            const auto b0 = m_bytes[m_offset++];
            const auto b1 = m_bytes[m_offset++];

            return static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(b0) << 8) |
                 static_cast<std::uint16_t>(b1)
            );
        }

        std::uint32_t read_u4()
        {
            ensure_available(4);

            const auto b0 = m_bytes[m_offset++];
            const auto b1 = m_bytes[m_offset++];
            const auto b2 = m_bytes[m_offset++];
            const auto b3 = m_bytes[m_offset++];

            return
                (static_cast<std::uint32_t>(b0) << 24) |
                (static_cast<std::uint32_t>(b1) << 16) |
                (static_cast<std::uint32_t>(b2) << 8)  |
                 static_cast<std::uint32_t>(b3);
        }

        std::span<const std::uint8_t> read_bytes(std::size_t count)
        {
            ensure_available(count);

            auto result = m_bytes.subspan(m_offset, count);
            m_offset += count;
            return result;
        }

        void skip(std::size_t count)
        {
            ensure_available(count);
            m_offset += count;
        }

        std::size_t offset() const
        {
            return m_offset;
        }

    private:
        void ensure_available(std::size_t count) const
        {
            if (m_offset + count > m_bytes.size()) {
                throw std::runtime_error("Unexpected end of class file");
            }
        }

        std::span<const std::uint8_t> m_bytes;
        std::size_t m_offset{};
    };
}
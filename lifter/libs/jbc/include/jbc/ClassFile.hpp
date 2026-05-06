#pragma once
// include/jbc/ClassFile.hpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <span>
#include <optional>

namespace jbc
{
    struct CpUtf8
    {
        std::string value;
    };

    struct CpClass
    {
        std::uint16_t nameIndex{};
    };

    struct CpNameAndType
    {
        std::uint16_t nameIndex{};
        std::uint16_t descriptorIndex{};
    };

    struct CpRef
    {
        std::uint16_t classIndex{};
        std::uint16_t nameAndTypeIndex{};
    };

    struct CpUnparsed
    {
        std::uint8_t tag{};
    };

    using ConstantPoolEntry = std::variant<
        std::monostate,   // index 0 is unused in JVM constant pools
        CpUtf8,
        CpClass,
        CpNameAndType,
        CpRef,
        CpUnparsed
    >;

    struct AttributeInfo
    {
        std::uint16_t nameIndex{};
        std::string name;
        std::vector<std::uint8_t> info;
    };

    struct MethodInfo
    {
        std::uint16_t accessFlags{};
        std::uint16_t nameIndex{};
        std::uint16_t descriptorIndex{};

        std::string name;
        std::string descriptor;

        std::vector<AttributeInfo> attributes;
        std::vector<std::uint8_t> bytecode;

        std::uint16_t maxStack;
        std::uint16_t maxLocals;
        std::uint32_t codeSize;
    };

    struct ClassFile
    {
        std::uint32_t magic{};
        std::uint16_t minorVersion{};
        std::uint16_t majorVersion{};

        std::vector<ConstantPoolEntry> constantPool;

        std::uint16_t accessFlags{};
        std::uint16_t thisClass{};
        std::uint16_t superClass{};

        std::string thisClassName;
        std::string superClassName;

        std::vector<MethodInfo> methods;

        MethodInfo getMethodInfo(int currentMethod) const
        {
            return methods[currentMethod];
        }

        int getNrOfMethods() const{
            return methods.size();
        }

        std::optional<std::string> tryGetUtf8(std::uint16_t index) const
        {
            if (index == 0 || index >= constantPool.size()) {
                return std::nullopt;
            }

            if (const auto* utf8 = std::get_if<jbc::CpUtf8>(&constantPool[index])) {
                return utf8->value;
            }

            return std::nullopt;
        }
    };

   

    ClassFile parse_class_file(std::span<const std::uint8_t> bytes);
}
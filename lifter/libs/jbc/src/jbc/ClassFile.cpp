// src/jbc/ClassFile.cpp
#include <jbc/ClassFile.hpp>

#include "jbc/ClassReader.hpp"

#include <stdexcept>
#include <string>
#include <iostream>

namespace jbc
{
    namespace
    {
        constexpr std::uint32_t JavaClassMagic = 0xCAFEBABE;

        const ConstantPoolEntry& cp_at(
            const ClassFile& file,
            std::uint16_t index
        )
        {
            if (index == 0 || index >= file.constantPool.size()) {
                throw std::runtime_error("Invalid constant pool index");
            }

            return file.constantPool[index];
        }

        const std::string& get_utf8(
            const ClassFile& file,
            std::uint16_t index
        )
        {
            const auto& entry = cp_at(file, index);

            if (const auto* utf8 = std::get_if<CpUtf8>(&entry)) {
                return utf8->value;
            }

            throw std::runtime_error("Expected CONSTANT_Utf8 entry");
        }

        std::string get_class_name(
            const ClassFile& file,
            std::uint16_t classIndex
        )
        {
            const auto& entry = cp_at(file, classIndex);

            if (const auto* classEntry = std::get_if<CpClass>(&entry)) {
                return get_utf8(file, classEntry->nameIndex);
            }

            throw std::runtime_error("Expected CONSTANT_Class entry");
        }

        std::string read_modified_utf8_lossy(ClassReader& reader)
        {
            const auto length = reader.read_u2();
            const auto bytes = reader.read_bytes(length);

            // Good enough for class names, method names, descriptors, etc.
            // Full JVM Modified UTF-8 decoding can come later.
            return std::string(
                reinterpret_cast<const char*>(bytes.data()),
                bytes.size()
            );
        }

        ConstantPoolEntry read_constant_pool_entry(ClassReader& reader)
        {
            const auto tag = reader.read_u1();

            switch (tag) {
            case 1: { // CONSTANT_Utf8
                return CpUtf8{ read_modified_utf8_lossy(reader) };
            }

            case 7: { // CONSTANT_Class
                return CpClass{ reader.read_u2() };
            }

            case 9:  // CONSTANT_Fieldref
            case 10: // CONSTANT_Methodref
            case 11: // CONSTANT_InterfaceMethodref
                return CpRef{
                    .classIndex = reader.read_u2(),
                    .nameAndTypeIndex = reader.read_u2()
                };

            case 12: // CONSTANT_NameAndType
                return CpNameAndType{
                    .nameIndex = reader.read_u2(),
                    .descriptorIndex = reader.read_u2()
                };

            case 3: // CONSTANT_Integer
            case 4: // CONSTANT_Float
                reader.skip(4);
                return CpUnparsed{ tag };

            case 5: // CONSTANT_Long
            case 6: // CONSTANT_Double
                reader.skip(8);
                return CpUnparsed{ tag };

            case 8: // CONSTANT_String
                reader.skip(2);
                return CpUnparsed{ tag };

            case 15: // CONSTANT_MethodHandle
                reader.skip(3);
                return CpUnparsed{ tag };

            case 16: // CONSTANT_MethodType
                reader.skip(2);
                return CpUnparsed{ tag };

            case 18: // CONSTANT_InvokeDynamic
                reader.skip(4);
                return CpUnparsed{ tag };

            case 19: // CONSTANT_Module
            case 20: // CONSTANT_Package
                reader.skip(2);
                return CpUnparsed{ tag };

            default:
                throw std::runtime_error(
                    "Unsupported constant pool tag: " + std::to_string(tag)
                );
            }
        }
    }

    void skip_member_info(ClassReader& reader)
    {
        reader.read_u2(); // access_flags
        reader.read_u2(); // name_index
        reader.read_u2(); // descriptor_index

        const auto attributesCount = reader.read_u2();

        for (std::uint16_t i = 0; i < attributesCount; ++i) {
            reader.read_u2();              // attribute_name_index
            const auto length = reader.read_u4();
            reader.skip(length);           // info
        }
    }

    AttributeInfo read_attribute(ClassReader& reader, const ClassFile& file)
    {
        AttributeInfo attribute;

        attribute.nameIndex = reader.read_u2();

        const auto length = reader.read_u4();
        const auto bytes = reader.read_bytes(length);

        attribute.info.assign(bytes.begin(), bytes.end());

        if (auto name = file.tryGetUtf8(attribute.nameIndex)) {
            attribute.name = std::string{ *name };
        }
        else {
            attribute.name = "<invalid attribute name>";
        }

        return attribute;
    }

    MethodInfo read_method(ClassReader& reader, const ClassFile& file)
    {
        MethodInfo method;

        method.accessFlags = reader.read_u2();
        method.nameIndex = reader.read_u2();
        method.descriptorIndex = reader.read_u2();

        if (auto name = file.tryGetUtf8(method.nameIndex)) {
            method.name = std::string{ *name };
        }
        else {
            method.name = "<invalid method name>";
        }

        if (auto descriptor = file.tryGetUtf8(method.descriptorIndex)) {
            method.descriptor = std::string{ *descriptor };
        }
        else {
            method.descriptor = "<invalid descriptor>";
        }

        const auto attributesCount = reader.read_u2();

        method.attributes.reserve(attributesCount);

        std::cout << "Number of attributes " << attributesCount << "\n";
        for (std::uint16_t i = 0; i < attributesCount; ++i) {
            AttributeInfo attribute = read_attribute(reader, file);
            if (attribute.name == "Code")
            {
                std::cout << "Code found\n";
                /*code.maxStack = reader.read_u2();
                code.maxLocals = reader.read_u2();

                const auto codeLength = reader.read_u4();
                const auto codeBytes = reader.read_bytes(codeLength);*/

                const auto codeBytes = attribute.info;
                method.maxStack = ((uint16_t)codeBytes[0] << 8) | codeBytes[1];
                method.maxLocals = ((uint16_t)codeBytes[2] << 8) | codeBytes[3];

                method.codeSize = ((uint32_t)codeBytes[4] << 24
                    | codeBytes[5] << 16
                    | codeBytes[6] << 8
                    | codeBytes[7]
                    );
                //method.bytecode.resize(method.codeSize);
                auto start = codeBytes.begin() + 8;
                auto end = start + method.codeSize;
                method.bytecode.assign(start,end);
            }
            else {
                method.attributes.push_back(attribute);
            }
        }

        return method;
    }

    ClassFile parse_class_file(std::span<const std::uint8_t> bytes)
    {
        ClassReader reader{ bytes };

        ClassFile file;

        file.magic = reader.read_u4();
        if (file.magic != JavaClassMagic) {
            throw std::runtime_error("Not a Java class file: invalid magic");
        }

        file.minorVersion = reader.read_u2();
        file.majorVersion = reader.read_u2();

        const auto constantPoolCount = reader.read_u2();

        file.constantPool.resize(constantPoolCount);
        file.constantPool[0] = std::monostate{};

        for (std::uint16_t i = 1; i < constantPoolCount; ++i) {
            file.constantPool[i] = read_constant_pool_entry(reader);

            // Long and Double consume two constant-pool slots.
            const auto* unparsed = std::get_if<CpUnparsed>(&file.constantPool[i]);
            if (unparsed && (unparsed->tag == 5 || unparsed->tag == 6)) {
                ++i;
                if (i < constantPoolCount) {
                    file.constantPool[i] = std::monostate{};
                }
            }
        }

        file.accessFlags = reader.read_u2();
        file.thisClass = reader.read_u2();
        file.superClass = reader.read_u2();

        file.thisClassName = get_class_name(file, file.thisClass);

        if (file.superClass != 0) {
            file.superClassName = get_class_name(file, file.superClass);
        }

        const auto interfacesCount = reader.read_u2();
        std::cout << "Reading " << interfacesCount << " number of interfaces.\n";
        for (std::uint16_t i = 0; i < interfacesCount; ++i) {
            reader.read_u2(); // interface_index
            
        }

        // fields
        const auto fieldsCount = reader.read_u2();
        for (std::uint16_t i = 0; i < fieldsCount; ++i) {
            skip_member_info(reader);
        }

        // methods
        const auto methodsCount = reader.read_u2();
        file.methods.reserve(methodsCount);

        for (std::uint16_t i = 0; i < methodsCount; ++i) {
            file.methods.push_back(read_method(reader, file));
        }

        // class-level attributes, skip for now
        const auto classAttributesCount = reader.read_u2();
        for (std::uint16_t i = 0; i < classAttributesCount; ++i) {
            reader.read_u2();
            const auto length = reader.read_u4();
            reader.skip(length);
        }

        return file;
    }
}
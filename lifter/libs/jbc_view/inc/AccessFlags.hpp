// include/jbc/AccessFlags.hpp
#pragma once

#include <cstdint>
#include <string>

namespace jbc
{
    inline std::string format_class_access_flags(std::uint16_t flags)
    {
        std::string result;

        auto add = [&](std::string_view name) {
            if (!result.empty()) {
                result += ' ';
            }
            result += name;
            };

        if (flags & 0x0001) add("public");
        if (flags & 0x0010) add("final");
        if (flags & 0x0020) add("super");
        if (flags & 0x0200) add("interface");
        if (flags & 0x0400) add("abstract");
        if (flags & 0x1000) add("synthetic");
        if (flags & 0x2000) add("annotation");
        if (flags & 0x4000) add("enum");
        if (flags & 0x8000) add("module");

        if (result.empty()) {
            return "-";
        }

        return result;
    }
}
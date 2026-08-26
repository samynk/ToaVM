#pragma once
#include <array>
#include <cstdint>
namespace asbc{
    struct Frame {
        // Parameters, locals and compiler-generated temporaries all live here.
        std::array<std::int32_t, 256> variables{};

        // Four-byte view of AngelScript's value register.
        std::uint32_t valueRegister{};

        // RET metadata. It will matter once we implement nested calls.
        std::uint16_t argumentWordsToPop{};

        bool running{true};
    };
}
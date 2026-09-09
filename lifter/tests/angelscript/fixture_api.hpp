#pragma once

#include <cmath>
#include <cstdint>

namespace fixture {
    inline int recorded = 0;
    inline float root(float value) { return std::sqrt(value); }
    inline float subtract(float a, float b) { return a - b; }
    inline float combine(std::int32_t a, float b, std::int32_t c) {
        return static_cast<float>(a) + b * static_cast<float>(c);
    }
    inline void record(std::int32_t value) { recorded = value; }
    inline float noargs() noexcept { return 9.5f; }
    inline float add(float a, float b) { return a + b; }
    inline std::uint32_t echo(std::uint32_t value) { return value; }
    inline void scalars(bool a, std::int8_t b, std::uint16_t c) {
        recorded = a ? b + c : -1;
    }
}

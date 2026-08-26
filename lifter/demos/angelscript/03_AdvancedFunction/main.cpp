#include <meta>
#include <iostream>

#include "asbc/decode.hpp"
#include "asbc/transform.hpp"
#include "asbc/function.hpp"

// float sqrHypotenuse(float,float)
inline constexpr std::array<std::uint32_t, 9> sqrHypotenuse{
    0x0000003Fu,
    0x0001007Au,
    0x00000000u,
    0x0002007Au,
    0xFFFFFFFFu,
    0x00016178u,
    0x00020001u,
    0x00016552u,
    0x0002650Au
};



int main()
{
    constexpr auto pFnHypotenuse = asbc::invoke<sqrHypotenuse, float,float, float>;
    constexpr float result = pFnHypotenuse(3.2f,4.1f);
    static_assert(result == 3.2f*3.2f+4.1f*4.1f);
    std::cout << "Result :" << result << std::endl;
    return 0;
}
#include <meta>
#include <iostream>

#include "asbc/decode.hpp"
#include "asbc/transform.hpp"
#include "asbc/function.hpp"

// int square(int)
inline constexpr std::array<std::uint32_t, 5> square_bytecode_0{
    0x0000003Fu,
    0x00010075u,
    0x00000000u,
    0x00010052u,
    0x0001000Au
};

constexpr std::int32_t generatedSquare(std::int32_t value)
{
    using FrameType = asbc::Frame<std::int32_t>;
    FrameType frame(value);

    constexpr std::meta::info program =
        asbc::createBlockInfo<FrameType, square_bytecode_0>();

    [:program:](frame);

    return std::bit_cast<std::int32_t>(
        frame.valueRegister
    );
}

static_assert(generatedSquare(0) == 0);
static_assert(generatedSquare(7) == 49);
static_assert(generatedSquare(-8) == 64);

int main()
{
    constexpr auto pFnSquare = asbc::invoke<square_bytecode_0, std::int32_t, std::int32_t>;
    constexpr int result = pFnSquare(13);
    static_assert(result == 169);

    constexpr auto offsets =
        std::define_static_array(
            asbc::createInstructionOffsets<
                square_bytecode_0
            >()
        );

    static_assert(offsets.size() == 4);
    static_assert(offsets[0] == 0);
    static_assert(offsets[1] == 1);
    static_assert(offsets[2] == 3);
    static_assert(offsets[3] == 4);

    constexpr auto suspend =
        asbc::decodeOperands<square_bytecode_0>(offsets[0]);

    static_assert(suspend.arg0 == 0);
    static_assert(suspend.arg1 == 0);
    static_assert(suspend.arg2 == 0);

    uint16_t arg0 = static_cast<uint16_t>(0x00010075u >> 16);
    std::cout << "arg 0 " << arg0 << std::endl;

    uint16_t arg2 = asbc::unsignedHighWord(0x00010075u);
    std::cout << "arg 2 " << arg2 << std::endl;

    constexpr auto multiply =
        asbc::decodeOperands<square_bytecode_0>(offsets[1]);

    static_assert(multiply.arg0 == 1);
    static_assert(multiply.arg1 == 0);
    static_assert(multiply.arg2 == 0);

    constexpr auto copyReturn =
        asbc::decodeOperands<square_bytecode_0>(offsets[2]);

    static_assert(copyReturn.arg0 == 1);

    constexpr auto returnInstruction =
        asbc::decodeOperands<square_bytecode_0>(offsets[3]);

    static_assert(returnInstruction.arg0 == 1);

    std::cout
        << "MULi dst=" << multiply.arg0
        << " src1=" << multiply.arg1
        << " src2=" << multiply.arg2
        << '\n';

    std::cout
        << "generatedSquare(12) = "
        << generatedSquare(12)
        << '\n';
    
    return 0;
}
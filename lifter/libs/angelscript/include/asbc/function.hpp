#include "asbc/frame.hpp"
#include "asbc/transform.hpp"

namespace asbc{
    template<
        auto const& Bytecode,
        typename Return,
        typename... Parameters
    >
    constexpr Return invoke(Parameters... parameters)
    {
        using FrameType = Frame<Parameters...>;
        FrameType frame{parameters...};

        constexpr std::meta::info program =
            createBlockInfo<
                FrameType,
                Bytecode
            >();

        [:program:](frame);

        if constexpr (!std::same_as<Return, void>) {
            return std::bit_cast<Return>(
                frame.valueRegister
            );
        }
    }
}
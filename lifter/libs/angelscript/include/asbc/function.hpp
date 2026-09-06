#include "asbc/frame.hpp"
#include "asbc/transform.hpp"

namespace asbc{
    template<
        typename HostApi,
        auto const& Bytecode,
        typename ReturnType,
        typename... Parameters
    >
    constexpr ReturnType invoke(Parameters... parameters)
    {
        using FrameType = Frame<HostApi, ReturnType,Parameters...>;
        FrameType frame{parameters...};

        constexpr std::meta::info program =
            createBlockInfo<
                FrameType,
                Bytecode
            >();

        [:program:](frame);

        if constexpr (!std::same_as<ReturnType, void>) {
            return std::bit_cast<ReturnType>(
                frame.valueRegister
            );
        }
    }
}
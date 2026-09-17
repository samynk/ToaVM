#include <asbc/function.hpp>
#include <asbc/format/function_reader.hpp>

#include <cstdint>
#include <iostream>

using Frame = asbc::Frame<asbc::standalone_environment, std::int32_t>;

// First construct the loop by hand, just like the earlier straight-line demo.
//constexpr auto test = asbc::condition<Frame, asBC_JS,
//    asbc::block<Frame, asbc::execute<Frame, asBC_CMPIi, 3, 10, 0>>>;

//constexpr auto body = asbc::block<Frame,
//    asbc::execute<Frame, asBC_ADDi, 1, 1, 3>,
//    asbc::execute<Frame, asBC_IncVi, 3>>;

/*
constexpr std::int32_t handwritten()
{
    Frame frame;
    asbc::block<Frame,
        asbc::execute<Frame, asBC_SetV4, 1, 0, 0>,
        asbc::execute<Frame, asBC_SetV4, 3, 0, 0>,
        asbc::loop<Frame, test, body>,
        asbc::execute<Frame, asBC_CpyVtoR4, 1>,
        asbc::execute<Frame, asBC_RET>>(frame);
    return frame.getReturnValue();
}*/

#if !__has_embed(<demo.asbc>)
#error "demo.asbc was not generated or AS_RES_DIR is not configured correctly"
#endif

inline constexpr unsigned char demo_asbc[] = {
#embed <demo.asbc>
};

//static_assert(asbc::format::inspectFunction<demo_asbc, 5>().name.equals("loopTest"));

constexpr std::int32_t lifted()
{
    return asbc::invoke<asbc::standalone_environment,
        asbc::format::decodedFunctionByteCode<demo_asbc, 5>, std::int32_t>();
}

//static_assert(handwritten() == 45);
//static_assert(lifted() == 45);

int main()
{
    return lifted();
    // std::cout << "Handwritten loop: " << handwritten() << '\n'
    //          << "Lifted loopTest: " << lifted() << '\n';
}

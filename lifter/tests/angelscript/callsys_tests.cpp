#include "fixture_api.hpp"
#include "callsys_fixture.hpp"
#include <asbc/execute.hpp>

#ifdef TOAVM_TEST_REFLECTION
#include <asbc/function.hpp>
#endif

#include <cstdlib>
#include <iostream>

using HostApi = asbc::host_api<
    asbc::bind<"scalars", &fixture::scalars>,
    asbc::bind<"echo", &fixture::echo>,
    asbc::bind<"noargs", &fixture::noargs>,
    asbc::bind<"other::subtract", &fixture::add>,
    asbc::bind<"record", &fixture::record>,
    asbc::bind<"combine", &fixture::combine>,
    asbc::bind<"subtract", &fixture::subtract>,
    asbc::bind<"sqrt", &fixture::root>>;

template<auto const& Asbc>
inline constexpr auto module = asbc::format::readSimpleModule<Asbc>();

template<auto const& Asbc>
using Environment = asbc::execution_environment<module<Asbc>, HostApi>;

static_assert(module<fixture_asbc>.function_count == 10);
static_assert(std::tuple_size_v<decltype(module<fixture_asbc>.used_functions)> == 8);
static_assert(!module<fixture_asbc>.debug_info_stripped);
static_assert(module<fixture_stripped_asbc>.debug_info_stripped);
static_assert(std::get<0>(module<fixture_asbc>.used_functions).signature.name.equals("sqrt"));
static_assert(std::get<1>(module<fixture_asbc>.used_functions).signature.name.equals("subtract"));
static_assert(std::get<2>(module<fixture_asbc>.used_functions).signature.parameter_types.size() == 3);
static_assert(std::get<5>(module<fixture_asbc>.used_functions).signature.name_space.equals("other"));
static_assert(std::tuple_size_v<decltype(module<plain_asbc>.used_functions)> == 0);

// DWORD operands must round-trip even when either signed half has its sign bit set.
inline constexpr std::array<std::uint32_t, 2> wideIndexProgram{asBC_CALLSYS, 0xfedcba98u};
inline constexpr auto wideOperands = asbc::decodeOperands<wideIndexProgram>();
static_assert(asbc::joinOperandWords(wideOperands.arg0, wideOperands.arg1) == 0xfedcba98u);

using SqrtTraits = asbc::detail::host_function_traits<decltype(&fixture::root)>;
static_assert(SqrtTraits::matches(std::get<0>(module<fixture_asbc>.used_functions).signature));
static_assert(!asbc::detail::host_function_traits<int (*)(float)>::matches(
    std::get<0>(module<fixture_asbc>.used_functions).signature));
static_assert(!asbc::detail::host_function_traits<float (*)(int)>::matches(
    std::get<0>(module<fixture_asbc>.used_functions).signature));
static_assert(!asbc::detail::host_function_traits<float (*)(float, float)>::matches(
    std::get<0>(module<fixture_asbc>.used_functions).signature));
static_assert(!asbc::detail::host_function_traits<double (*)(double)>::supported);
static_assert(!asbc::detail::host_function_traits<void (*)(int&)>::supported);
static_assert([]() consteval {
    auto signature = std::get<0>(module<fixture_asbc>.used_functions).signature;
    signature.parameter_types[0].flags = 4; // reference
    return !SqrtTraits::matches(signature);
}());

void require(bool condition) {
    if (!condition) throw "CALLSYS regression check failed";
}

// Exercise the actual decoded instructions on C++20 compilers too. Only the
// reflection-based assembly of the block is replaced by an index-sequence fold.
template<auto const& Program, std::size_t I, class Frame>
constexpr void step(Frame& frame) {
    constexpr auto offset = []() consteval {
        return asbc::createInstructionOffsets<Program>()[I];
    }();
    constexpr auto op = asbc::decodeOpcode(Program[offset]);
    constexpr auto args = asbc::decodeOperands<Program>(offset);
    asbc::execute<Frame, op, args.arg0, args.arg1, args.arg2>(frame);
}

template<auto const& Asbc, std::size_t Function, class R, class... Args>
R run(Args... args) {
#ifdef TOAVM_TEST_REFLECTION
    return asbc::invoke<Environment<Asbc>,
        asbc::format::decodedFunctionByteCode<Asbc, Function>, R, Args...>(args...);
#else
    constexpr auto& program = asbc::format::decodedFunctionByteCode<Asbc, Function>;
    constexpr auto count = []() consteval {
        return asbc::createInstructionOffsets<program>().size();
    }();
    asbc::Frame<Environment<Asbc>, R, Args...> frame{args...};
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (step<program, I>(frame), ...);
    }(std::make_index_sequence<count>{});
    require(frame.stackSize() == 0);
    if constexpr (!std::is_void_v<R>) return frame.getReturnValue();
#endif
}

template<auto const& Asbc>
void checkModule() {
    require(run<Asbc, 0, float>(3.0f, 4.0f) == 5.0f);
    require(run<Asbc, 1, float>(6.25f) == 2.5f);
    require(run<Asbc, 2, float>(8.75f, 2.25f) == 6.5f);
    require(run<Asbc, 3, float>(std::int32_t{-4}, 2.5f, std::int32_t{3}) == 3.5f);
    require(run<Asbc, 4, float>(81.0f, 16.0f) == 5.0f);
    fixture::recorded = 0;
    run<Asbc, 5, void>(std::int32_t{-17});
    require(fixture::recorded == -17);
    require(run<Asbc, 6, float>() == 9.5f);
    require(run<Asbc, 7, float>(8.75f, 2.25f) == 11.0f);
    require(run<Asbc, 8, std::uint32_t>(0xfedcba98u) == 0xfedcba98u);
}

int main() {
    try {
        checkModule<fixture_asbc>();
        checkModule<fixture_stripped_asbc>();
        require(run<plain_asbc, 0, std::int32_t>(std::int32_t{7}) == 49);

        using Frame = asbc::Frame<Environment<fixture_asbc>, float>;
        Frame frame;
        frame.pushDWord(0x12345678u); // live value below the call's arguments
        frame.pushDWord(std::bit_cast<asbc::dword>(2.25f));
        frame.pushDWord(std::bit_cast<asbc::dword>(8.75f));
        asbc::execute<Frame, asBC_CALLSYS, 1>(frame);
        require(frame.getReturnValue() == 6.5f && frame.stackSize() == 1);
        require(frame.popDWord() == 0x12345678u);

        frame.setReturnValue(0x12345678u);
        frame.pushDWord(static_cast<asbc::dword>(-23));
        asbc::execute<Frame, asBC_CALLSYS, 3>(frame);
        require(frame.stackSize() == 0 && fixture::recorded == -23);
        require(frame.valueRegister == 0x12345678u); // void leaves the register alone

        frame.pushDWord(513);
        frame.pushDWord(0x123456f9u); // low byte is int8(-7); high bits are irrelevant
        frame.pushDWord(0x12345601u); // low byte is bool(true)
        asbc::execute<Frame, asBC_CALLSYS, 7>(frame);
        require(frame.stackSize() == 0 && fixture::recorded == 506);

        frame.pushDWord(0x12345678u);
        bool underflow = false;
        try { asbc::execute<Frame, asBC_CALLSYS, 1>(frame); }
        catch (const char*) { underflow = true; }
        require(underflow && frame.stackSize() == 1 && frame.popDWord() == 0x12345678u);

        // Frame access to parameter zero and negative offsets preserves bits.
        asbc::Frame<Environment<fixture_asbc>, float, float, float> parameters{1.25f, -2.5f};
        asbc::execute<decltype(parameters), asBC_PshV4, 0>(parameters);
        require(parameters.popDWord() == std::bit_cast<asbc::dword>(1.25f));
        parameters.setReturnValue(std::bit_cast<asbc::dword>(3.75f));
        asbc::execute<decltype(parameters), asBC_CpyRtoV4, -1>(parameters);
        require(parameters.get<float, -1>() == 3.75f);

        std::cout << "CALLSYS checks passed (debug and stripped AngelScript bytecode)\n";
        return EXIT_SUCCESS;
    } catch (const char* error) {
        std::cerr << error << '\n';
        return EXIT_FAILURE;
    }
}

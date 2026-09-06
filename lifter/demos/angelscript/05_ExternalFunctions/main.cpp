#include <cmath>
#include <iostream>

#include <asbc/host_api.hpp>
#include "asbc/instruction.hpp"
#include "asbc/transform.hpp"
#include "asbc/function.hpp"
#include <asbc/format/module_reader.hpp>
#include <asbc/format/function_reader.hpp>

float host_sqrt(float value)
{
    return std::sqrt(value);
}

void print(float value)
{
    std::cout << value << '\n';
}

using HostApi = asbc::host_api<
    asbc::bind<"sqrt", &host_sqrt>,
    asbc::bind<"print", &print>
>;



static_assert(HostApi::contains<"sqrt">);
static_assert(!HostApi::contains<"sin">);

constexpr auto sqrtFunction =
    HostApi::get<"sqrt">();

float result =
    HostApi::invoke<"sqrt">(25.0f);

// Also verifies the signature:
constexpr auto checkedSqrt =
    HostApi::resolve<"sqrt", float(float)>();



#if !__has_embed(<demo.asbc>)
#error "demo.asbc was not generated or AS_RES_DIR is not configured correctly"
#endif

inline constexpr unsigned char demo_asbc[] = {
#embed <demo.asbc>
};

inline constexpr auto demo_module =
    asbc::format::inspect_simple_module(demo_asbc);

static_assert(!demo_module.debug_info_stripped);
static_assert(demo_module.function_count == 3);

using Environment =
    asbc::execution_environment<demo_module, HostApi>;

int main()
{
    //

    asbc::Frame<Environment,float,float,float> f(3.2f,4.1f);
    f.pushDWord(std::bit_cast<asbc::dword>(3.2f));
    asbc::dword value = f.popDWord();
    float check = std::bit_cast<float>(value);
        std::cout << "Check: " << check << "\n";

    f.setReturnValue(value);
    float returnValue = f.getReturnValue();

    std::cout << "Return Value: " << returnValue << "\n";


    float result2 = asbc::invoke<Environment, asbc::format::decodedFunctionByteCode<demo_asbc, 2>,
        float,float,float>(3.2f,4.1f);
    std::cout << "Result : " << result2 << "\n";
    return 0;
}
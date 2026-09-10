#include <cmath>
#include <iostream>

#include <asbc/host_api.hpp>
#include "asbc/instruction.hpp"
#include "asbc/transform.hpp"
#include "asbc/function.hpp"
#include "asbc/frame.hpp"
#include <asbc/format/module_reader.hpp>
#include <asbc/format/function_reader.hpp>
#include <asbc/format/used_function_reader.hpp>

float host_atan2(float y, float x)
{
    return std::atan2(y, x);
}

float host_sqrt(float value)
{
    return std::sqrt(value);
}

void host_print(std::int32_t value)
{
    std::cout << value << '\n';
}

using HostApi = asbc::host_api<
    asbc::bind<"sqrt", &host_sqrt>,
    asbc::bind<"print", &host_print>,
    asbc::bind<"atan2", &host_atan2>
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
    asbc::format::readSimpleModule<demo_asbc>();

static_assert(!demo_module.debug_info_stripped);
static_assert(demo_module.function_count > 4);
static_assert(std::get<0>(demo_module.used_functions).signature.name.equals("sqrt"));

using Environment = asbc::execution_environment<demo_module, HostApi>;

int main()
{
    constexpr auto usedFunctions = asbc::format::inspectUsedFunctions<demo_asbc>();
    for (const auto& function : usedFunctions) {
        std::cout << "Used function " << function.index << ": ";
        for (unsigned char c : function.signature.name) {
            std::cout << static_cast<char>(c);
        }
        std::cout << '\n';
    }

    float result2 = asbc::invoke<Environment, asbc::format::decodedFunctionByteCode<demo_asbc, 2>,
        float,float,float>(3.2f,4.1f);
    std::cout << "Result : " << result2 << "\n";

    float angle = asbc::invoke<Environment, asbc::format::decodedFunctionByteCode<demo_asbc, 3>,
        float,float,float>(2.87f, 3.23f);

    std::cout << "Angle : " << angle << "\n";
    std::cout << "Angle check : " << std::atan2(2.87f, 3.23f) << "\n";

    // sum function
    int sum =
        asbc::invoke<
            asbc::standalone_environment,
            asbc::format::decodedFunctionByteCode<demo_asbc, 4>,
            std::int32_t
        >();

        std::cout << "Sum : " << sum << "\n";

    return 0;
}
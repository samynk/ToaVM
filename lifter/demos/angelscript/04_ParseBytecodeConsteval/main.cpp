#include <meta>
#include <iostream>

#include "asbc/instruction.hpp"
#include "asbc/transform.hpp"
#include "asbc/function.hpp"
#include <asbc/format/module_reader.hpp>
#include <asbc/format/function_reader.hpp>

#if !__has_embed(<demo.asbc>)
#error "demo.asbc was not generated or AS_RES_DIR is not configured correctly"
#endif

inline constexpr unsigned char demo_asbc[] = {
#embed <demo.asbc>
};

inline constexpr auto demo_module =
    asbc::format::inspect_simple_module(demo_asbc);

static_assert(!demo_module.debug_info_stripped);
static_assert(demo_module.function_count == 4);

static_assert(demo_module.first_function.name.size != 0);

 inline constexpr auto mulByteCode =
    asbc::format::readFirstFunctionByteCode<demo_asbc>();

int main()
{
    std::cout << "First function :" ;

     for (const unsigned char c : demo_module.first_function.name) {
        std::cout << static_cast<char>(c);
    }
    std::cout << "\n";

    int result = asbc::invoke<mulByteCode,std::int32_t,std::int32_t>(5);
    std::cout << "Result : " << result << "\n";

    return 0;
}
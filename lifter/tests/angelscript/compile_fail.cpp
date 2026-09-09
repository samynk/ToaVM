#include "fixture_api.hpp"
#include "callsys_fixture.hpp"
#include <asbc/execute.hpp>

inline constexpr auto module = []() consteval {
    auto result = asbc::format::readSimpleModule<fixture_asbc>();
#if FAILURE_CASE == 5
    std::get<0>(result.used_functions).origin = asbc::format::used_function_origin::module;
#endif
    return result;
}();

#if FAILURE_CASE == 1
using Api = asbc::host_api<>;
#elif FAILURE_CASE == 2
using Api = asbc::host_api<asbc::bind<"sqrt", &fixture::root>, asbc::bind<"sqrt", &fixture::root>>;
#elif FAILURE_CASE == 3
using Api = asbc::host_api<asbc::bind<"sqrt", &fixture::subtract>>;
#else
using Api = asbc::host_api<asbc::bind<"sqrt", &fixture::root>>;
#endif

int main() {
    using Env = asbc::execution_environment<module, Api>;
    using Frame = asbc::Frame<Env, float>;
    Frame frame;
#if FAILURE_CASE == 4
    asbc::execute<Frame, asBC_CALLSYS, 99>(frame);
#else
    asbc::execute<Frame, asBC_CALLSYS, 0>(frame);
#endif
}

#include <asbc/control_flow.hpp>
#include <asbc/format/function_reader.hpp>

#if FAILURE_CASE == 1
inline constexpr std::array<std::uint32_t, 1> program{asBC_JMP};
#elif FAILURE_CASE == 2
inline constexpr std::array<std::uint32_t, 3> program{asBC_JMP, 100, asBC_RET};
#elif FAILURE_CASE == 3
inline constexpr std::array<std::uint32_t, 3> program{asBC_JMP, 0xfffffffcu, asBC_RET};
#elif FAILURE_CASE == 4
inline constexpr std::array<std::uint32_t, 5> program{
    asBC_JMP, 1, asbc::format::packOpcodeWord(asBC_SetV4, 1), 0, asBC_RET};
#elif FAILURE_CASE == 5
inline constexpr std::array<std::uint32_t, 2> program{asBC_JMP, 0xfffffffeu};
#elif FAILURE_CASE == 6
inline constexpr auto program = [] {
    std::array<std::uint32_t, 3> code{asBC_JMP, 100, asBC_RET};
    asbc::format::restoreJumpOffsets(code);
    return code;
}();
#elif FAILURE_CASE == 7
inline constexpr std::array<std::uint32_t, 1> program{0xffu};
#endif

inline constexpr auto flow = asbc::createControlFlow<program>();

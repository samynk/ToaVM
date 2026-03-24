#include <array>
#include <meta>
#include <ranges>
#include <iostream>

enum Op : uint8_t {
    PUSH = 0x01,
    ADD  = 0x02,
    SUB  = 0x03,
    HALT = 0xFF
};

constexpr std::array<uint8_t,8> program  {
    PUSH, 5,
    PUSH, 3,
    ADD, 0,
    HALT, 0
};

struct CPU {
    uint8_t A{0};
    uint8_t B{0};
    uint8_t PC{0};
    bool zero_flag{false};
    std::array<uint8_t, 8> stack{};
    uint8_t sp{0};
} ;

template<Op op, uint8_t arg = 0>
void execute(CPU& cpu) {
    if constexpr (op == PUSH) {
        cpu.stack[cpu.sp++] = arg;
    } else if constexpr (op == ADD) {
        auto a = cpu.stack[--cpu.sp];
        auto b = cpu.stack[--cpu.sp];
        cpu.stack[cpu.sp++] = a + b;
    } else if constexpr (op == SUB) {
        auto a = cpu.stack[--cpu.sp];
        auto b = cpu.stack[--cpu.sp];
        cpu.stack[cpu.sp++] = a - b;
    } else if constexpr (op == HALT) {
        // nothing
    }
}

consteval auto createProgram()
{
    constexpr auto tmpl = ^^execute;
    std::array<void(*)(CPU&), 4> steps{};
    size_t pc = 0;
    using namespace std::meta;
    template for (constexpr int idx : std::views::iota(decltype(steps.size()){ 0 }, steps.size())) {
        constexpr Op op = static_cast<Op>(program[2*idx]);
        constexpr uint8_t arg = program[2*idx+1];
        
        // reflect the enum value and substitute into template
        constexpr auto spec = substitute(tmpl, {reflect_constant(op),reflect_constant(arg)});
        steps[idx] = [:spec:];
    }
    return steps;
}  

int main(void){
    constexpr auto program = createProgram();
    CPU cpu;
    for (auto step : program){
        step(cpu);
       
    }
    return cpu.stack[0];
}

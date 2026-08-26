#include <angelscript.h>

#include "asbc/host_api.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace {

void message_callback(const asSMessageInfo* message, void*)
{
    std::string_view kind = "info";
    if (message->type == asMSGTYPE_WARNING) {
        kind = "warning";
    } else if (message->type == asMSGTYPE_ERROR) {
        kind = "error";
    }

    std::cerr << message->section << ':' << message->row << ':' << message->col
              << ": " << kind << ": " << message->message << '\n';
}

std::string read_file(const char* path)
{
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error{std::string{"Could not open script: "} + path};
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

bool check(int result, std::string_view operation)
{
    if (result >= 0) {
        return true;
    }

    std::cerr << operation << " failed with AngelScript error " << result << '\n';
    return false;
}

class InputBytecodeStream final : public asIBinaryStream {
public:
    explicit InputBytecodeStream(const char* path)
        : file_(path, std::ios::binary)
    {
    }

    int Read(void* ptr, asUINT size) override
    {
        file_.read(static_cast<char*>(ptr), static_cast<std::streamsize>(size));
        return static_cast<int>(file_.gcount());
    }

    int Write(const void*, asUINT) override
    {
        return 0;
    }

private:
    std::ifstream file_;
};

} // namespace

void print_arguments(
    const asDWORD* instruction,
    asEBCType type
)
{
    switch (type) {
    case asBCTYPE_NO_ARG:
        break;

    case asBCTYPE_W_ARG:
        std::cout << " arg=" << asBC_WORDARG0(instruction);
        break;

    case asBCTYPE_wW_ARG:
        std::cout << " dst=" << asBC_SWORDARG0(instruction);
        break;

    case asBCTYPE_rW_ARG:
        std::cout << " src=" << asBC_SWORDARG0(instruction);
        break;

    case asBCTYPE_wW_rW_ARG:
        std::cout
            << " dst=" << asBC_SWORDARG0(instruction)
            << " src=" << asBC_SWORDARG1(instruction);
        break;

    case asBCTYPE_rW_rW_ARG:
        std::cout
            << " src1=" << asBC_SWORDARG0(instruction)
            << " src2=" << asBC_SWORDARG1(instruction);
        break;

    case asBCTYPE_wW_rW_rW_ARG:
        std::cout
            << " dst="  << asBC_SWORDARG0(instruction)
            << " src1=" << asBC_SWORDARG1(instruction)
            << " src2=" << asBC_SWORDARG2(instruction);
        break;

    case asBCTYPE_DW_ARG:
        std::cout << " arg=" << asBC_DWORDARG(instruction);
        break;

    case asBCTYPE_QW_ARG:
        std::cout << " arg=" << asBC_QWORDARG(instruction);
        break;

    default:
        std::cout << " <operand format "
                  << static_cast<int>(type)
                  << " not decoded yet>";
        break;
    }
}

void emit_constexpr_bytecode(
    asIScriptFunction& function,
    std::string_view array_name,
    std::ostream& output
)
{
    static_assert(
        sizeof(asDWORD) == sizeof(std::uint32_t),
        "The bytecode exporter expects 32-bit AngelScript DWORDs"
    );

    asUINT length = 0;
    const asDWORD* bytecode = function.GetByteCode(&length);

    if (bytecode == nullptr) {
        std::cerr
            << "No bytecode available for "
            << function.GetDeclaration()
            << '\n';

        return;
    }

    const auto old_flags = output.flags();
    const auto old_fill = output.fill();

    output
        << "// " << function.GetDeclaration() << '\n'
        << "inline constexpr std::array<std::uint32_t, "
        << length << "> "
        << array_name
        << "{\n";

    for (asUINT index = 0; index < length; ++index) {
        output
            << "    0x"
            << std::hex
            << std::uppercase
            << std::setw(8)
            << std::setfill('0')
            << static_cast<std::uint32_t>(bytecode[index])
            << "u";

        if (index + 1 != length) {
            output << ',';
        }

        output << '\n';
    }

    output << "};\n\n";

    output.flags(old_flags);
    output.fill(old_fill);
}

int main(int argc, char** argv)
{ 
    asIScriptEngine* engine = asCreateScriptEngine();
    register_host_api(*engine);

    asIScriptModule* module = engine->GetModule("Demo", asGM_ALWAYS_CREATE);
    InputBytecodeStream stream{"resources/demo.asbc"};

    if ( module->LoadByteCode(&stream) < 0 ) {
        std::cerr << "Failed to load bytecode from demo.asbc\n";
        return EXIT_FAILURE;
    }

    for (asUINT i=0; i < module->GetFunctionCount(); ++i) {
        asIScriptFunction* func = module->GetFunctionByIndex(i);
        std::cout << "Function " << i << ": " << func->GetDeclaration() << '\n';

        asUINT length =0;
        asDWORD* instruction = func->GetByteCode(&length);
        asDWORD* end = instruction + length;

        asUINT pc = 0;

        while (instruction < end) {
            const auto opcode =
                static_cast<asEBCInstr>(
                    *reinterpret_cast<const asBYTE*>(instruction)
                );

            const auto& info = asBCInfo[opcode];
            const asUINT instruction_size = asBCTypeSize[info.type];

            std::cout << pc
                    << ": " << info.name << "[ " << instruction_size << " DWORDs ]";
            print_arguments(instruction, info.type);
            std::cout << '\n';
            // std::cout << " [" << instruction_size << " DWORDs]\n";

            instruction += instruction_size;
            pc += instruction_size;
        }
        emit_constexpr_bytecode(*func, "bytecode_" + std::to_string(i), std::cout);
    }

}

#include <angelscript.h>

#include "asbc/host_api.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

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

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error{"Could not open input file: " + path.string()};
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

class OutputBytecodeStream final : public asIBinaryStream {
public:
    explicit OutputBytecodeStream(const std::filesystem::path& path)
        : output_{path, std::ios::binary}
    {
    }

    int Write(const void* data, asUINT size) override
    {
        output_.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
        return 0;
    }

    int Read(void*, asUINT) override
    {
        // SaveByteCode only writes. A runtime loader would implement this method.
        return 0;
    }

    [[nodiscard]] bool good() const
    {
        return output_.good();
    }

private:
    std::ofstream output_;
};

void print_usage(const char* executable)
{
    std::cerr << "Usage: " << executable
              << " <output.asbc> <input.as> [additional-input.as ...]\n";
}

void print_host_api(const asIScriptEngine& engine)
{
    std::cout << "Registered host functions:\n";

    for (asUINT index = 0; index < engine.GetGlobalFunctionCount(); ++index) {
        const asIScriptFunction* function =
            engine.GetGlobalFunctionByIndex(index);

        if (function == nullptr) {
            continue;
        }

        std::cout
            << "  " << function->GetId()
            << ": "
            << function->GetDeclaration(
                   true,  // include object name
                   true,  // include namespace
                   true)  // include parameter names
            << '\n';
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::filesystem::path output_path = argv[1];
    asIScriptEngine* engine = asCreateScriptEngine();
    if (engine == nullptr) {
        std::cerr << "Could not create the AngelScript engine\n";
        return EXIT_FAILURE;
    }

    const auto finish = [&engine](int status) {
        engine->ShutDownAndRelease();
        return status;
    };

    if (engine->SetMessageCallback(
            asFUNCTION(message_callback), nullptr, asCALL_CDECL) < 0 ||
        !register_host_api(*engine)) {
        std::cerr << "Could not configure the AngelScript engine\n";
        return finish(EXIT_FAILURE);
    }
    print_host_api(*engine);
    try {
        asIScriptModule* module = engine->GetModule("compiled_module", asGM_ALWAYS_CREATE);

        // All input files become sections of one AngelScript module and therefore
        // one bytecode file. AddScriptSection copies the source text.
        for (int index = 2; index < argc; ++index) {
            const std::filesystem::path input_path = argv[index];
            const std::string source = read_file(input_path);

            if (module->AddScriptSection(
                    input_path.string().c_str(), source.c_str(), source.size()) < 0) {
                std::cerr << "Could not add script section: " << input_path << '\n';
                return finish(EXIT_FAILURE);
            }
        }

        if (module->Build() < 0) {
            std::cerr << "AngelScript compilation failed\n";
            return finish(EXIT_FAILURE);
        }

        OutputBytecodeStream bytecode{output_path};
        if (!bytecode.good()) {
            std::cerr << "Could not open output file: " << output_path << '\n';
            return finish(EXIT_FAILURE);
        }

        if (module->SaveByteCode(&bytecode) < 0 || !bytecode.good()) {
            std::cerr << "Could not save bytecode: " << output_path << '\n';
            return finish(EXIT_FAILURE);
        }

        std::cout << "Compiled " << (argc - 2) << " script section(s) to "
                  << output_path << '\n';
        return finish(EXIT_SUCCESS);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return finish(EXIT_FAILURE);
    }
}


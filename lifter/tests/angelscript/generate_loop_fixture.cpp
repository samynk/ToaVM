#include <angelscript.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
struct Stream : asIBinaryStream {
    std::vector<unsigned char> bytes;
    int Read(void*, asUINT) override { return -1; }
    int Write(const void* data, asUINT size) override {
        const auto* first = static_cast<const unsigned char*>(data);
        bytes.insert(bytes.end(), first, first + size);
        return 0;
    }
};

void check(int result) {
    if (result < 0) throw std::runtime_error("AngelScript loop fixture generation failed");
}

void message(const asSMessageInfo* info, void*) {
    std::cerr << info->section << ':' << info->row << ": " << info->message << '\n';
}

void save(std::ostream& out, asIScriptModule& module, const char* name, bool stripped) {
    Stream stream;
    check(module.SaveByteCode(&stream, stripped));
    out << "inline constexpr unsigned char " << name << "[] = {\n";
    for (auto byte : stream.bytes) out << static_cast<unsigned>(byte) << ',';
    out << "\n};\n";
}

// Canonicalize unused padding fields; compare every meaningful DWORD against
// GetByteCode(), independently of ToaVM's saved-bytecode reader.
void saveInstructions(std::ostream& out, asIScriptFunction& function, asUINT index) {
    asUINT size = 0;
    const auto* code = function.GetByteCode(&size);
    out << "inline constexpr std::array<std::uint32_t, " << size
        << "> engine_bytecode_" << index << "{\n";
    for (asUINT pc = 0; pc < size;) {
        const auto op = static_cast<asEBCInstr>(code[pc] & 0xffu);
        const auto type = asBCInfo[op].type;
        std::uint32_t first = op;
        // RET's argument is deliberately saved as zero by SaveByteCode; the
        // engine restores platform-specific argument-pop metadata when loading.
        if (type != asBCTYPE_NO_ARG && type != asBCTYPE_DW_ARG && op != asBC_RET)
            first |= code[pc] & 0xffff0000u;
        out << first << "u,";
        const auto words = asBCTypeSize[type];
        if (words == 2) {
            auto second = code[pc + 1];
            if (type == asBCTYPE_rW_rW_ARG) second &= 0xffffu;
            out << second << "u,";
        } else if (words != 1) {
            throw std::runtime_error("Unexpected loop fixture operand format");
        }
        pc += words;
    }
    out << "\n};\n";
}
}

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    auto* engine = asCreateScriptEngine();
    if (!engine) return 1;
    try {
        check(engine->SetMessageCallback(asFUNCTION(message), nullptr, asCALL_CDECL));
        auto* module = engine->GetModule("loops", asGM_ALWAYS_CREATE);
        check(module->AddScriptSection("loops.as", R"(
            int loopTest() {
                int sum = 0;
                for (int i = 0; i < 10; ++i) { sum += i; }
                return sum;
            }
            int sumBelow(int limit) {
                int sum = 0;
                for (int i = 0; i < limit; ++i) { sum += i; }
                return sum;
            }
            int choose(int x) {
                int result = 3;
                if (x < 0) { result = 7; } else { result = 11; }
                return result;
            }
            int early(int limit) {
                int sum = 0;
                for (int i = 0; i < 10; ++i) {
                    if (i == limit) { return sum; }
                    sum += i;
                }
                return sum;
            }
            int nested() {
                int sum = 0;
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 4; ++j) { sum += i; }
                }
                return sum;
            }
            int doOnce(int limit) {
                int i = 0;
                do { ++i; } while (i < limit);
                return i;
            }
            int negativeStart() {
                int sum = 0;
                for (int i = -3; i < 0; ++i) { sum += i; }
                return sum;
            }
            int nonnegative(int x) {
                int result = 2;
                if (x >= 0) { result = 5; }
                return result;
            }
        )"));
        check(module->Build());

        auto* context = engine->CreateContext();
        auto expect = [&](const char* name, int expected, bool hasArg = false, int arg = 0) {
            check(context->Prepare(module->GetFunctionByName(name)));
            if (hasArg) check(context->SetArgDWord(0, static_cast<asDWORD>(arg)));
            if (context->Execute() != asEXECUTION_FINISHED ||
                static_cast<int>(context->GetReturnDWord()) != expected) {
                throw std::runtime_error("Unexpected AngelScript loop result");
            }
        };
        expect("loopTest", 45);
        expect("sumBelow", 0, true, 0);
        expect("sumBelow", 0, true, 1);
        expect("sumBelow", 45, true, 10);
        expect("choose", 7, true, -1);
        expect("choose", 11, true, 0);
        expect("early", 6, true, 4);
        expect("early", 45, true, 11);
        expect("nested", 12);
        expect("doOnce", 1, true, 0);
        expect("doOnce", 10, true, 10);
        expect("negativeStart", -6);
        expect("nonnegative", 2, true, -1);
        expect("nonnegative", 5, true, 0);
        context->Release();

        std::ofstream out(argv[1]);
        out << "// Generated by AngelScript 2.38.0; do not edit.\n"
            << "#pragma once\n#include <array>\n#include <cstdint>\n";
        save(out, *module, "loop_asbc", false);
        save(out, *module, "loop_stripped_asbc", true);
        for (asUINT i = 0; i < module->GetFunctionCount(); ++i) {
            saveInstructions(out, *module->GetFunctionByIndex(i), i);
        }
        out.close();
        if (!out) throw std::runtime_error("Could not write loop fixture");
        engine->ShutDownAndRelease();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        engine->ShutDownAndRelease();
        return 1;
    }
}

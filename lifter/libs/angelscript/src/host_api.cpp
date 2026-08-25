#include "asbc/host_api.hpp"

#include <angelscript.h>

#include <iostream>

namespace {

void print(int value)
{
    std::cout << "AngelScript says: " << value << '\n';
}

} // namespace

bool register_host_api(asIScriptEngine& engine)
{
    return engine.RegisterGlobalFunction(
               "void print(int)", asFUNCTION(print), asCALL_CDECL) >= 0;
}


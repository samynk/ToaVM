#include "asbc/host_api.hpp"

#include <angelscript.h>

#include <iostream>
#include <cmath>


void print(int value)
{
    std::cout << "AngelScript says: " << value << '\n';
}

bool register_host_api(asIScriptEngine& engine)
{
    engine.RegisterGlobalFunction("float sqrt(float)", asFUNCTION(std::sqrtf), asCALL_CDECL);
    return engine.RegisterGlobalFunction(
               "void print(int)", asFUNCTION(print), asCALL_CDECL) >= 0;
    
}
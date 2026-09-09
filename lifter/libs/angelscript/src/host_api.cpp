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
    return engine.RegisterGlobalFunction(
               "float sqrt(float)", asFUNCTIONPR(std::sqrt, (float), float), asCALL_CDECL) >= 0 &&
           engine.RegisterGlobalFunction(
               "void print(int)", asFUNCTION(print), asCALL_CDECL) >= 0 &&
            engine.RegisterGlobalFunction(
                "float atan2(float,float)", asFUNCTION(print), asCALL_CDECL) >= 0;
    
}

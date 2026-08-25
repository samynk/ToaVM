#pragma once

class asIScriptEngine;

// The bytecode compiler and runtime must register an identical application API.
// Add future host functions, types, and properties here for both executables.
bool register_host_api(asIScriptEngine& engine);


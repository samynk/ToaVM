# Calling application functions

The serialized `CALLSYS` operand is an index into the ASBC **used-function
table**. It is independent of both AngelScript's runtime function IDs and the
order of bindings in `host_api`.

Read the metadata before creating the execution environment:

```cpp
#include <asbc/format/used_function_reader.hpp>
#include <asbc/function.hpp>
#include <asbc/host_api.hpp>

// demo_asbc is the embedded byte array; HostApi contains the application's bindings.
inline constexpr auto demo_module =
    asbc::format::readSimpleModule<demo_asbc>();

using Environment = asbc::execution_environment<demo_module, HostApi>;

// In the demo module, used-function index 0 is sqrt.
constexpr auto& used = std::get<0>(demo_module.used_functions);
static_assert(used.signature.name.equals("sqrt"));

float result = asbc::invoke<Environment,
    asbc::format::decodedFunctionByteCode<demo_asbc, 2>,
    float, float, float>(3.0f, 4.0f); // 5.0f
```

`inspect_simple_module` remains the lightweight header inspection API.
`readSimpleModule` additionally reads through the used-function table. The
reader keeps the string and data-type caches alive while consuming script
records, global-function references, and the intervening sections.

For metadata alone:

- `usedFunctionCount<Bytes>()` returns the number of used functions.
- `inspectUsedFunctions<Bytes>()` returns an array of summaries, including
  origin, name, namespace, return type, and parameter count.
- `readUsedFunction<Bytes, Index>()` returns one complete signature.
- `readUsedFunctions<Bytes>()` returns a tuple with exact-sized parameter arrays.

`CALLSYS` builds the qualified name (for example `math::sqrt`), selects the
function from `host_api` at compile time, and verifies the ASBC signature. It
rejects missing or duplicate bindings, mismatched signatures, invalid indices,
and entries that are not application functions. Names must be unique within
`host_api`; overload selection by signature is not implemented.

Arguments are pushed right to left by AngelScript, so argument zero is at the
top of the operand stack. A braced tuple sequences the typed pops left to right
before calling the bound C++ function. Only the call's arguments are removed;
other values remain on the stack. Non-void results go into the value register,
and void calls leave that register unchanged. There is no runtime name lookup.

The call boundary currently supports by-value `bool`, 8-, 16-, and 32-bit
integers, and `float`: each uses one DWORD stack slot. It also supports `void`
returns and `noexcept` function pointers. Wider values, references, pointers,
objects, methods, and template function signatures are rejected. The surrounding reader/executor still
requires every instruction in the script to belong to its supported opcode
subset. Imported function bindings and used object types remain unsupported.

## Regression checks

The focused tests build independently of the reflection compiler and FTXUI:

```sh
cmake -S lifter/tests/angelscript -B build/callsys
cmake --build build/callsys -j
ctest --test-dir build/callsys --output-on-failure
```

To use an existing AngelScript 2.38.0 checkout, add
`-DFETCHCONTENT_SOURCE_DIR_ANGELSCRIPT=/path/to/angelscript` at configuration.
With GCC 16+ and a CMake version supporting C++26, also set
`-DTOAVM_TEST_REFLECTION=ON` to exercise the actual `asbc::invoke` path.

The generator uses AngelScript's `SaveByteCode` with and without debug
information and verifies representative results in AngelScript's own VM.
The C++20 runner executes the real decoded `execute` specializations using an
index-sequence fold in place of reflection's block assembly. Tests cover
mixed and multiple arguments, nested calls, namespaced bindings, zero-argument
and void functions, registration-order independence, bit preservation, exact
stack cleanup, underflow, and expected compile-time failures.

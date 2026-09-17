# AngelScript loops and jumps

The AngelScript lifter now turns supported jump patterns into native C++
`loop`, `doLoop`, and `branch` specializations, composed by `block`.
The control-flow plan is constructed at compile time. Execution has no
bytecode program counter or opcode dispatch loop.

The `06_Loops` demo first constructs the sum loop by hand, then invokes
`loopTest` from the embedded `demo.asbc`. Both have a
`static_assert(result == 45)`.

## Presentation example

`CMPIi` compares a signed local with a signed 32-bit immediate and writes
−1, 0, or 1 to the value register. `JS` branches when that register is negative.
The comparison uses relational operators, avoiding subtraction overflow.

The in-memory jump operand is a signed DWORD displacement relative to the
instruction immediately after the jump:

| Jump at DWORD | Displacement | Target DWORD |
| --- | ---: | ---: |
| 6: JMP | +6 | 8 + 6 = 14 |
| 17: JS | −11 | 19 − 11 = 8 |

The unsigned display `4294967285` represents −11. The disassembler now prints
the signed displacement and target, and decodes `CMPIi` as `src=3 value=10`.

The resulting arrangement is:

```cpp
using Frame = asbc::Frame<asbc::standalone_environment, std::int32_t>;

constexpr auto test = asbc::condition<Frame, asBC_JS,
    asbc::block<Frame, asbc::execute<Frame, asBC_CMPIi, 3, 10, 0>>>;

constexpr auto body = asbc::block<Frame,
    asbc::execute<Frame, asBC_ADDi, 1, 1, 3>,
    asbc::execute<Frame, asBC_IncVi, 3>>;

// Between the initialization and return steps:
// asbc::loop<Frame, test, body>(frame);
```

`test` runs the comparison setup on every evaluation. Because the entry
`JMP` reaches the condition first, the body can execute zero times.

## Reading and lowering

AngelScript's `SaveByteCode` stores jump distances in **instructions**.
`GetByteCode` exposes distances in **DWORDs**. The ASBC reader first reconstructs
all instructions, then translates each saved displacement using the instruction
boundaries. The lowerer therefore accepts the same jump representation from
an embedded ASBC file or a hand-authored DWORD array.

The added instruction support is `IncVi`, `CMPi`, `CMPIi`, `JMP`,
`JZ`, `JNZ`, `JS`, `JNS`, `JP`, and `JNP`.
The conditional jumps test the signed 32-bit value register for zero,
nonzero, negative, nonnegative, positive, and nonpositive, respectively.

`control_flow.hpp` recognizes:

- The layout used by AngelScript for the example's pre-test loop:
  a forward jump to straight-line condition setup, followed by a conditional
  back edge to the body.
- Conditional back edges for do/while loops.
- Forward conditional branches for if and if/else, including nesting.
- Ordinary forward jumps and jumps to a shared return epilogue.

`transform.hpp` recursively lowers this plan using reflection.
`block` and the control-flow templates respect `frame.running`, so a
`RET` inside a nested construct also stops its enclosing function.

Targets outside the function, targets inside operand words, truncated
instructions, and unsupported opcodes are rejected at compile time.
Control flow that cannot fit the supported regions is also rejected.
There is no general support yet for break/continue, compound loop conditions,
unconditional back edges, switch jump tables, or irreducible control flow.
Instructions used inside each region must belong to the implemented subset.

## Verification

Build the focused tests independently of the presentation projects:

```sh
cmake -S lifter/tests/angelscript -B build/jumps
cmake --build build/jumps -j
ctest --test-dir build/jumps --output-on-failure
```

With GCC 16 and a CMake version supporting C++26, enable the real reflection
lowerer as well:

```sh
cmake -S lifter/tests/angelscript -B build/jumps-reflection \
  -DCMAKE_CXX_COMPILER=g++-16 -DTOAVM_TEST_REFLECTION=ON
cmake --build build/jumps-reflection -j
ctest --test-dir build/jumps-reflection --output-on-failure
```

The fixture generator compiles eight functions with AngelScript 2.38.0 and
checks representative results in its VM. The tests compare the decoded
instructions with `GetByteCode` after normalizing unused padding and the
platform-specific RET argument, which AngelScript saves as zero.
They cover debug and stripped ASBC, zero/one/many iterations, a runtime bound,
negative immediates, signed comparison extremes, all six conditional predicates,
if/else, nested loops, do/while, and early returns. Expected compilation failures
check malformed jumps and unsupported control flow. Existing CALLSYS tests
remain part of the same suite.

On C++20, a test-only template lowering consumes the same production
control-flow plan and calls the production execution/control-flow templates.
That validates the reader, structuring, and execution; it does not validate
the reflection API calls. `TOAVM_TEST_REFLECTION=ON` tests those calls through
`asbc::invoke`.

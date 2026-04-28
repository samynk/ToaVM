# Tradeoffs

This document captures the honest case for and against compile-time bytecode
lifting via C++26 reflection — the approach this project explores. It is
deliberately two-sided: any technique that's worth taking seriously has
failure modes, and understanding them up front is how you know when the
technique is the right answer and when it isn't.

The target use case is embedding a scripting language (JVM bytecode today,
Lua next) in a C++ host — primarily a game engine, though the properties
generalize.

---

## Advantages

### Unified toolchain

Lifted bytecode is compiled by the same C++ compiler, with the same flags,
as the host program. There is no second compilation pipeline to configure,
tune, or debug. `-O2` vs `-O3`, LTO on or off, PGO profiles, target CPU
selection, sanitizer choices, C++ standard library — all apply uniformly
to native and lifted code. Contrast with any JIT or separate AOT compiler,
which makes its own optimization decisions independent of the host's.

### Cross-boundary optimization

Because lifted code is ordinary C++ functions in the same translation
unit (or at least the same LTO domain) as the host, the compiler can
inline across the boundary in both directions. A host function calling
a lifted method can have that method inlined. A lifted method calling a
host helper gets that helper inlined. Neither is possible with traditional
FFI (JNI, Lua C API) or a runtime JIT — those boundaries are hard walls
to the optimizer.

### Zero runtime compilation cost

All analysis — decode, CFG construction, structural reduction, code
emission — happens at C++ compile time. Startup latency for lifted code
is zero. Cold-start-sensitive deployments (serverless, CLI tools,
embedded boot) benefit directly. There is no warmup phase, no tier-up,
no deoptimization.

### No runtime memory cost for the compiler

A JIT ships megabytes of compilation infrastructure in the deployed
binary (LLVM alone is ~30MB). This approach ships none of it — the
"compiler" was the C++ toolchain on the developer's machine, which
isn't in the runtime image. For embedded and memory-constrained
environments this can be the difference between feasible and not.

### Works where JITs don't

Console platforms (Switch, PlayStation, Xbox) enforce W+X memory
protection that forbids runtime code generation. iOS historically
has similar restrictions. Hardened server environments, kernel code,
freestanding C++ environments, and some regulatory contexts rule out
JITs entirely. This approach produces ordinary native code and works
wherever C++ works.

### Portability inherits from C++

Every platform with a conforming C++26 compiler is a supported target
by construction. No per-architecture backend to write, no per-OS
porting work beyond what the host C++ code already does. This
generalizes to exotic targets (WASM, GPU via CUDA/SYCL if squinted at
correctly) that would require bespoke work for any traditional JIT or
AOT compiler.

### Determinism across platforms

Same source plus same flags produces bit-identical output across
platforms (modulo platform-specific native code where unavoidable).
For lockstep multiplayer, deterministic replay systems, reproducible
builds, and regulated environments, this is valuable by construction.
JITs make runtime decisions that vary with profile data, making
cross-platform determinism hard.

### Ordinary debugging experience

Lifted code runs as real C++ functions with real symbols in a real
call stack. Debuggers, profilers, sanitizers (ASan, UBSan, TSan),
coverage tools, and performance counters all work uniformly across
native and lifted code. No separate "script debugger" alongside the
C++ debugger. Cross-boundary stack traces are legible end-to-end.

### Security posture matches the host

No W+X pages, no writable code regions, no JIT spray attack surface.
The security properties of the deployed binary are exactly the
properties of an ordinary ahead-of-time C++ build. For hardened
deployments, this removes an entire category of attack surface that
JITs introduce.

### Aggressive static optimization becomes possible

For bytecode where inputs are statically known, the C++ compiler can
apply the full force of its optimizer — constant folding, loop
unrolling, scalar evolution, dead code elimination. In the best case
(fully static program), a lifted loop can fold to a compile-time
constant. This isn't a guaranteed property for every program, but
it's reachable for the right shape of program in a way no JIT can
match because JITs specialize on runtime data, not absent data.

### Separation of concerns mirrors language philosophy

The JVM specification deliberately keeps `javac` non-optimizing,
trusting the JIT to handle optimization later. This approach slots
into that philosophy cleanly: bytecode stays faithful to source, the
C++ compiler handles optimization statically. The layering is clean
and each component does one job.

---

## Disadvantages

### Compile-time cost

Lifting non-trivial bytecode meaningfully increases C++ compilation
time. Template instantiation, reflection metadata, and inlining of
structural primitives are expensive in ways that scale poorly with
method size and program complexity. A real game with thousands of
scripts will need a strategy for this — separate translation units
per script, incremental builds, or dev-mode fallback to an interpreter.

### Cannot handle runtime-loaded code

Any bytecode not available at the C++ compiler's build time cannot
be lifted. Plugin systems, user-generated content, hot-patched code,
network-delivered scripts — none of these work. A traditional
interpreter or JIT is still required for dynamic workloads, and the
lifted approach complements rather than replaces them.

### Not faster than a mature JIT on profile-sensitive code

A tracing JIT with runtime profile information will outperform this
approach on code where profile genuinely matters: polymorphic call
sites, type-specialized numeric code, adaptive inlining based on
observed targets.

### Inlining budgets push back at scale

The "lifted code looks like hand-written C++" property depends on
the C++ compiler fully inlining the structural primitives and
per-opcode `execute<>` templates. Compilers have inlining budgets
based on code size, call depth, and enclosing function complexity.
For small methods (< 100 instructions) this works transparently. For
large methods, some `execute<>` calls stay as real calls, which
breaks the register-machine SROA optimization that makes the stack
array disappear. Mitigations exist (`always_inline` attributes,
flattening) but they're not free.

### Irreducible control flow requires fallback

Structural analysis relies on the CFG being reducible — collapsible
to a single region through standard transformations. Most compiler
output is reducible; hand-written bytecode, certain obfuscators, and
some specific language features (JVM `JSR`/`RET`, unstructured
`goto` in Lua 5.2+) can produce irreducible graphs that don't fit
any region pattern. The approach needs a flat PC-dispatch fallback
for these cases, which then doesn't get the optimization benefits.

### Metaprogramming ergonomics are rough

C++26 reflection is powerful but the development experience is
worse than ordinary C++: no debugger, no print statements,
substitution failures that produce wall-of-text error messages,
compile-time bugs that only surface as wrong runtime behavior.
Bring-up is slower than equivalent runtime code. A dual-mode design
(runtime interpreter plus compile-time lifter) is almost required
to keep the project tractable.

### Bleeding-edge compiler dependency

P2996 reflection is new. Compiler support is partial, varies by
vendor and version, and has bugs. Shipping a project that depends
on it today means narrow toolchain support and some risk of
encountering compiler issues that are genuinely not your bug to fix.
This improves monotonically as compilers mature but is a real cost
in the short term.

### Opaque optimized output

Once the C++ compiler has inlined, fused, and optimized the lifted
code, the resulting assembly bears no resemblance to the original
bytecode. Debugging at the "which bytecode instruction is executing
right now" level is essentially impossible in optimized builds. This
is the same tradeoff as debugging optimized C++ — fine if you accept
that debug and release are different modes, problematic if you want
source-level visibility in release.

### Build-time exposure of source code

Bytecode must be available at host build time, which means it's
available to anyone with access to the build. For applications where
scripts are intellectual property distinct from the host (modding
APIs with proprietary scripts, licensed content), the traditional
"ship bytecode, interpret at runtime" model gives a weaker form of
separation that this approach cannot replicate.

### Tight coupling to host toolchain

The host's compiler decisions propagate to lifted code — which is
usually a benefit, but means the lifted code cannot make independent
choices. If the host is compiled poorly (wrong flags, missing
optimization, broken toolchain), lifted performance suffers with
no recourse. There is no fallback layer that can route around host
problems.

### Ecosystem maturity

Mature scripting language integrations have decades of tooling:
debuggers, profilers, package managers, IDE plugins, community
knowledge. This approach starts from zero on all of those. The
underlying C++ tooling helps, but anything scripting-specific
(script-level profiling aggregation, script package management,
authoring environments that understand the lifted form) needs to
be built or foregone.

### Not all bytecode maps cleanly to structured C++

Some bytecode patterns — complex exception handling, coroutines,
`invokedynamic`, continuations, generators — don't have clean
structural representations. Handling them requires either extending
the region tree with new kinds (each one a non-trivial piece of
design work) or falling back to flat dispatch (losing optimization).
The approach scales well to straightforward structured code and
progressively less well as the source language's control flow
becomes more exotic.

---

## When this approach is the right answer

- Scripting code is authored alongside the host and shipped with it
- Performance of scripting code genuinely matters (not just "works")
- Target platforms include JIT-hostile environments (consoles,
  hardened systems, freestanding)
- Determinism across platforms is valued
- The host team is already invested in C++ toolchain mastery
- Debug and release modes can legitimately differ

## When it isn't

- Scripts are loaded at runtime from untrusted sources
- Modding or user-generated content is a first-class use case
- The scripting code base is large enough that compile-time cost
  becomes a development bottleneck
- Target platforms lack a C++26-capable toolchain
- A mature existing scripting solution already satisfies requirements
  and the cost of migration exceeds the benefit
- The team doesn't have the appetite for bleeding-edge C++ reflection

---

## Framing

This approach does not replace JITs or traditional interpreters. It
fills a quadrant of the design space they don't cover: ahead-of-time,
portable to any C++ target, zero runtime footprint, no boundary
between host and guest. For workloads that fit the quadrant, the
combination of properties is unavailable elsewhere. For workloads
that don't, a JIT or interpreter remains the right tool and can
coexist with this approach in a hybrid system.

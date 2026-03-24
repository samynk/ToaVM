This first example demonstrates a tiny stack based language with fixed width opcodes:

    constexpr std::array<uint8_t,8> program  {
        PUSH, 5,
        PUSH, 3,
        ADD, 0,
        HALT, 0
    };

This makes it easier to loop over the bytecode in the ``createProgram`` function.

To run this example on GodBolt.org, use the **x86-64 clang (reflection-C++26)** compiler and compile with the following flags:

    -O2 -freflection-latest

The entire program should collapse into the following assembly code:

    main:
        mov     eax, 8
        ret

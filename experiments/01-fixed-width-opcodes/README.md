This first example demonstrates a tiny stack based language with fixed width opcodes:

    constexpr std::array<uint8_t,8> program  {
        PUSH, 5,
        PUSH, 3,
        ADD, 0,
        HALT, 0
    };

This makes it easier to loop over the bytecode.

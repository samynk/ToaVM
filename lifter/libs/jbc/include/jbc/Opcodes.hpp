namespace jbc {
    enum Op : uint8_t {
        // Constants
        ICONST_M1 = 0x02,
        ICONST_0 = 0x03,
        ICONST_1 = 0x04,
        ICONST_2 = 0x05,
        ICONST_3 = 0x06,
        ICONST_4 = 0x07,
        ICONST_5 = 0x08,
        BIPUSH = 0x10,

        // Loads (local → stack)
        ILOAD_0 = 0x1a,
        ILOAD_1 = 0x1b,
        ILOAD_2 = 0x1c,
        ILOAD_3 = 0x1d,

        // Stores (stack → local)
        ISTORE_0 = 0x3b,
        ISTORE_1 = 0x3c,
        ISTORE_2 = 0x3d,
        ISTORE_3 = 0x3e,

        // Arithmetic
        IADD = 0x60,
        ISUB = 0x64,
        IMUL = 0x68,

        // Jumps
        GOTO = 0xa7,
        IF_ICMPLT = 0xa1,

        // Misc
        IINC = 0x84,

        // Return
        IRETURN = 0xac,
    };

    // Returns the number of operand bytes following the opcode
    consteval int getOpInfo(Op op) {
        switch (op) {
        case BIPUSH:    return 1;  // 1-byte operand
        case GOTO:      return 2;  // 2-byte signed offset
        case IF_ICMPLT: return 2;  // 2-byte signed offset
        case IINC:      return 2;  // local index + increment
        default:        return 0;
        }
    }
}
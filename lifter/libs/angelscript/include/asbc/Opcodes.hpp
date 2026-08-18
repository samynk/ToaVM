#include <cstdint>
namespace jbc
{
    enum Op : uint8_t
    {
        // Constants, Stack and Local variables
        PshC4,
        PshC8,
        PshV4,
        PshV8,
        PshG4,
        SetV4,
        SetV8,
        Pop,

        // Control flow
        JMP,
        JZ,
        JNZ,
        CALL,
        CALLSYS,
        CALLBND,
        RET,
        // Arithmetic
        ADDi,
        ADDf,
        SUBi,
        SUBf,
        MULi,
        MULf,
        DIVi,
        DIVf,
        INCi,
        DECi,
        CMPi,
        CMPf,
        BAND,
        BOR,
        BXOR,

        // Object and pointer manipulation
        PshObj,
        RefCpy,
        ChkRefS,
        CallObj,
        Free,

        // Conversions
        iTOf,
        fTOi,
        iTOd,
        dTOi
    };

    // Returns the number of operand bytes following the opcode
    consteval int getOpInfo(Op op)
    {
        switch (op)
        {
        case BIPUSH:
            return 1; // 1-byte operand
        case GOTO:
            return 2; // 2-byte signed offset
        case IF_ICMPLT:
            return 2; // 2-byte signed offset
        case IINC:
            return 2; // local index + increment
        default:
            return 0;
        }
    }
}
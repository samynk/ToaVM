To run this example on GodBolt.org, use the **x86-64 clang (reflection-C++26)** compiler and compile with the following flags:

    -O2 -freflection-latest

The entire program should collapse into the following assembly code:

    main:
        mov     eax, 18
        ret

In this experiment, we throw away the program counter in the CPU struct. Instead of jumping around in an array of executable statements, I now use the control flow mechanisms that are already present in the language.

- a block template that accepts a variadic list of function pointers that are executed left to right.
- an icmplt template that also accepts a variadic list of executable steps. At the end of this execution there must be two additional numbers on the stack which are then compared. 
- a loop that accepts an icmplt as condition and a block to execute (corresponding to the body of the loop).

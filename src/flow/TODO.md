
### BUGS

- runtime:
  - call-args verifier gets invoked on IR-nodes not on AST nodes.
    this enables us to pre-apply optimizations on each call arg.
    - requires CallInstr/etc to also provide a source location for diagnostic printing.
- parser:
  - `var i = 42; i = i / 2;` second stmt fails due to regex parsing attempts.
- general
  - memory leaks wrt. Flow IR destruction (check valgrind in general)
  - cannot dump vm program when not linked

### NEW Features

- method overloading (needs updates to symbol lookup, not just by name but by signature)
  - allows us to provide multiple implementations for example: workers(I)V and workers(i)V
- performance:
  - new opcode: ASCONST to reference a string array from the constant pool
  - new opcode: ANCONST to reference an int array from the constant pool
  - new opcode: ALOCAL to reference an array from within the local data pool
    - then rewrite call arg handling to use ALOCAL instead of IMOV on AllocaInstr.arraySize() > 1

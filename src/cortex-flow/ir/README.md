# Linear IR

- IR optimizations
  - constant folding
  - same-BasicBlock merging
  - dead code elimination
  - jump threading
  - efficient register allocator

### Class Hierarchy

````
- IRProgram
- IRBuilder
  - IRGenerator
- VMCodeGenerator
- Value
  - BasicBlock            SSA-conform code sequence block
  - IRVariable            a writable variable
  - Constant              Base class for constants
    - ConstantValue<T>
      - ConstantString    <Buffer>
      - ConstantIP        <IPAddress>
      - ConstantCidr      <Cidr>
      - ConstantRegExp    <RegExp>
      - IRHandler
  - Instr
    - PhiNode             merge branched values 
    - AllocaInstr         field/array allocation
    - StoreInstr          store to variable
    - LoadInstr           load from variable
    - BranchInstr
      - CondBrInstr       conditional jump
      - BrInstr           unconditional jump
```

### Equations

```
Dom(n)      = {n} ∪ ( ⋂ Dom(m) )                                   | page 478
                    m ⋲ preds(n)

IDom(n)     = Dom(n) \ {n}

                                          ________
LiveOut(n)  = ⋃ (UEVar(m) ∩ (LiveOut(m) ∩ VarKill(m)))             | page 446.
            m ⋲ succ(n)

UEVar(n)    =

VarKill(n)  = 

```

### Mathematical Unicode Symbols

- ∣   U+2223
- ∩   U+2229
- ∪   U+222A
- ⋲   U+22F2
- ⋃   U+22C3
- ⋂   U+22C2

### Useful References

 * http://en.wikipedia.org/wiki/Static_single_assignment_form
 * http://en.wikipedia.org/wiki/Basic_block
 * http://en.wikipedia.org/wiki/Use-define_chain
 * http://en.wikipedia.org/wiki/Compiler_optimization
 * http://llvm.org/doxygen/classllvm_1_1Value.html

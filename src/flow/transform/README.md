
contains a set of IR transformation algorithms.

### empty block elimination

Removes blocks that contain just a single jump instruction by relinking
all predecessors to the successor of the empty block.

#### dead block elimination

Removes blocks hat have no predecessors.

#### stupid instruction rewriter

- `condbr %cond, %fooBB, %fooBB` which will jump to `%fooBB` no matter of the result of `%cond`.
  - rewritten into `br %fooBB`
- ...


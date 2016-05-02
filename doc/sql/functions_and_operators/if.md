
### if (expression)
Executes one of two subexpressions based on the return value of a conditional
expression.

    if(cond_expr, true_branch, false_branch)

If `cond_expr` is true, the `true_branch` expression will be executed and the result
returned. Otherwise the `false_branch` expression will be executed and the result
returned.

Examples:

    SELECT if(1 == 1, 23, 42);
    Result: 23

    SELECT if(1 == 2, 2 * 2, 4 * 4);
    Result: 16


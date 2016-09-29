9.2.5 Life of a Query
=====================

#### Step 1: Tokenizing (Query String -> Tokens)
A `Tokenizer` splits the query string into tokens (e.g TOKEN_LPAREN and TOKEN_IDENTIFIER)


#### Step 2: Parsing (Tokens -> AST Tree)

The list of tokens is handed to a `Parser` that transforms it into an abstract syntax
tree. The abstract syntax tree consists of generic `ASTNode`s


#### Step 3: Query Tree Building (AST Tree -> Query Tree)

From the AST, the `QueryTreeBuilder` builds another tree reprenstation of the query
called the QueryTree. A Query Tree consists of `QueryTreeNode`s.

So whats the differnce between the AST and the QueryTree? First of all the 
AST is a tree with generic nodes (each node only stores a name, the list of
it's child nodes and some other metadata). In the QueryTree, the nodes
describe much higher-level SQL semantics. Examples for QueryTree nodes are
"SequentialScanNode", "GroupByNode" and "JoinNode". Each node type is its
own C++ class that inherits from the generic `QueryTreeNode` and implements
a rich interface. You'd have to look at the code to get a feel for this.

Another way to see the query tree is that it doesn't describe the syntax
of the staement but the high level operations required to actually calculate
the result from the source tables. Each query node is more or less equivalent to
an abstract "table expression" (i.e. an expression
that takes a number of tables as input and returns another table).

Note that neither the tokenizer, nor the parser, nor the query tree builder
are pluggable/user configurable as their semantics are mostly dictated by the
SQL standard, so you shouldn't have to change this unless you want to extend
the SQL language itself.


### Step 4: Query Planning (Query Tree -> Query Plan)

The next step is to plan the query. A query plan consists of one or more
query trees.

Since the input to the query planning step is alreayd a fully functional query tree,
the most simple query planner is one that just returns the same query tree that it
received.

However, all the high level optimizations like inserting index and hash joins,
choosing join order or pushing down scan restrictions take place in this step.
All of these optimizations are basically functions that rewrite the query tree.

The query planning step is pluggable. All you need to do is to implement the
`QueryPlanner` interface (see the `DefaultQueryPlanner` for an example)


### Step 5: Query Execution

In the last step, the query plan gets passed to a `Scheduler` that executes it.
The input query tree is still a pretty abstract description of the query so the
scheduler has full freedom on how to execute actually the query.

Have a look at the `DefaultScheduler` and the `ClusterScheduler` to see two
different scheduler implementations. The default scheduler executes the query
in the currenly running thread, the cluster scheduler pushes some parts of the
query out to other nodes and parallelizes some operations.

The heavy lifting, however, is usually not done within the scheduler itself.
The scheduler merely coordinates what code should run where and the calls into
the standard runtime and table expression implementations that perform the actual
operations.

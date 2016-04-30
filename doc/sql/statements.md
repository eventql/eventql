Statements
==========

The following SQL statements are currently supported.

  - SELECT ... ;
  - INSERT INTO ... ;
  - DRAW ... ;
  - DESCRIBE ... / EXPLAIN ...;
  - SHOW TABLES;

SELECT Statement
----------------


    SELECT
        select_expr [, select_expr ...]
        [FROM table_references
            [WHERE predicate_expr]
            [GROUP BY {col_name | expr}, ...]
            [HAVING predicate_expr]
            [ORDER BY {col_name | expr} [ASC | DESC], ...]
            [LIMIT {[offset,] row_count | row_count OFFSET offset}]]


##### WITHIN RECORD extensions

...

##### GROUP BY and HAVING extensions

Like MySQL, fnordmetric SQL extends the use of GROUP BY so that the select list
can refer to nonaggregated columns not named in the GROUP BY clause. This assumes
that the nongrouped columns will have the same group-wise values. Otherwise, the
result is undefined. The same applies for the HAVING clause.

If you use a group function in a statement containing no GROUP BY clause, it
will emit a single group.


DRAW Statement
--------------

The DRAW statement allows you to specify that the query result should be returned
as a chart (or other visualization) rather than a table. Here is a simple example
of the DRAW statement:

    DRAW LINECHART
        WITH AXIS BOTTOM

    SELECT time AS x, value AS y
        FROM example_data;

Read the [documentation for the DRAW statement](/docs/sql/chartsql)

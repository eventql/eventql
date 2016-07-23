3.5 Inserting Records
=====================

Before you can insert records, you need to [create a table](../creating-tables/).
Note that all tables have a mandatory unique primary key.

### Insert via SQL

You can use the SQL `INSERT INTO` statement to insert new records.

    INSERT [INTO] table_name
      [(column_name, ...)] VALUES (expr, ...)
    | FROM JSON "{ ... }"

If you do not specify a list of columns, the columns are selected in the same
order as defined in the table schema. Please run `DESCRIBE table_name`, if
you're unsure about the order of the columns.

Example:

    INSERT INTO my_table (col1, col2) VALUES (123, "somestring");

Example (from JSON):

    INSERT INTO my_table FROM JSON '{"col1": 123, "col2": "somestring"}'


### Insert via HTTP API

You can also insert JSON records using the HTTP API. Please refer to the
[API reference for the specifics](/documentation/api/).

Example insert using the HTTP API:

    curl \
       -X POST \
       -d '[ { "table": "my_table", "data": { "col1": 123, "col2": "somestring" }  }  ]' \
       http://localhost:9175/api/v1/tables/insert


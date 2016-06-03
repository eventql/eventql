2.2.1 Inserting Records
=======================

Before you can insert records, you need to [create a table](../creating-tables/). Note that all tables
have a mandatory primary key. 

Two consecutive inserts with the same primary key value are treated as an insert
followed by an update (that is, every insert with a primary key value equal to
that of another record that already exists will replace that original record).

    INSERT INTO table_name
      (column_name, ...) VALUES (expr, ...)
    | FROM JSON "{ ... }"

Records can be insert as a list of values or JSON.

Insert a record as JSON:

    INSERT INTO my_table FROM JSON '{"col1": 123, "col2": "somestring"}'


Via the HTTP API, you can currently insert records as either JSON or Protobuf messages. Please
refer to the [documentation for your driver or API for the specifics](../../api/).

<br />

Example insert via the HTTP API:

    curl \
       -X POST \
       -d '[ { "table": "my_table", "data": { "col1": 123, "col2": "somestring" }  }  ]' \
       -H 'Authorization: Token <auth_token>' \
       https://api.eventql.io/api/v1/tables/insert


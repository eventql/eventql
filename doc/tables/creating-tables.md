3.3 Creating Tables
===================

    CREATE TABLE table_name
    ( column_definition, column_definition, ...)

    column_definition is:

    column_name sql_type
    | column_name column_type [ PRIMARY KEY ] [NOT NULL]
    | column_name REPEATED column_type
    | column_name [REPEATED] RECORD ( column_definition, column_definition, ...)
    | PRIMARY KEY ( column_name [, column_name1 , column_name2, ... ] )


sql_type must be one of the listed [SQL data types](../datatypes/).

A table must have at least one unique primary key whose first column is treated as partition key to distribute the rows among the hosts.
The partition key can be of type `string`, `uint64` or `datetime`. To learn more about primary keys and understand how to choose one
to get the best performance, read on the ["Partitioning" page](../partitioning/).

#### Example:

    CREATE TABLE temperature_measurements (
      collected_at    DATETIME,
      sensor_id       STRING,
      temperature     DOUBLE,
      PRIMARY KEY(time, sensor_id)
    );

**NOTE:** If you are an EventQL Cloud user you can also create and update table
schemas from the web interface. Go to `EventQL Cloud > Tables` and click the
`Create Table` button on the top right corner.

### HTTP API

You can also create tables using the HTTP API. Please refer to the [HTTP API reference](/documentation/api/http/).

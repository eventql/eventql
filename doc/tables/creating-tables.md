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

**NOTE for high-volume timeseries:** If you are planning to store large volumes
timeseries-structured data in the table, please see the
[Timeseries & Logs page](../../collecting-data/high-volume-timeseries-logs) for
tips to get the best performance.

#### Examples:

    CREATE TABLE users (
      user_id         UINT64,
      user_name       STRING,
      user_email      STRING
      PRIMARY KEY(user_id)
    );

    CREATE TABLE twitter_firehose (
      time            DATETIME,
      event_id        STRING,
      author          STRING,
      tweet           STRING,
      PRIMARY KEY(time, event_id)
    ) WITH finite_partition_size = 600000000;


### HTTP API

You can also create tables using the HTTP API. Please refer to the [HTTP API reference](/documentation/api/http/).

**NOTE:** If you are an EventQL Cloud user you can also create and update table
schemas from the web interface. Go to `EventQL Cloud > Tables` and click the
`Create Table` button on the top right corner.

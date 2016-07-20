2.1.3 Creating Tables
=====================

    CREATE TABLE table_name
    ( column_definition, column_definition, ...)

    column_definition is:

    column_name cql_type
    | column_name column_type [ PRIMARY KEY ] [NOT NULL]
    | column_name REPEATED column_type
    | column_name [REPEATED] RECORD ( column_definition, column_definition, ...)
    | PRIMARY KEY ( column_name [, column_name1 , column_name2, ... ] )


cql_type must be one of the listed [SQL data types](/documentation/collecting-and-storing/tables/datatypes).

A table must have at least one unique primary that must not be of type RECORD.
EventQL treats the first column of the primary key as partition key to distribute the rows among the hosts.
While EventQL supports STRING, UINT64 and DATETIME columns as partition key, it is strongly recommended to choose a column of type DATETIME as partition key for timeseries data.

<br />
**NOTE:** If you are an EventQL Cloud user you can also create and update table
schemas from the web interface. Go to `EventQL Cloud > Tables` and click the
`Create Table` button on the top right corner.

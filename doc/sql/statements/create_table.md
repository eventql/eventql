5.2.4 CREATE TABLE
==================

    CREATE TABLE table_name
    ( column_definition, column_definition, ...)

    column_definition is:

    column_name sql_type
    | column_name column_type [ PRIMARY KEY ] [NOT NULL]
    | column_name REPEATED column_type
    | column_name [REPEATED] RECORD ( column_definition, column_definition, ...)
    | PRIMARY KEY ( column_name [, column_name1 , column_name2, ... ] )


**NOTE for high-volume timeseries:** If you are planning to store large volumes
timeseries-structured data in the table, please see the
[Timeseries & Logs page](../../../collecting-data/high-volume-timeseries-logs) for
tips to get the best performance.

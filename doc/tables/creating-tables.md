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

cql_type is a type, other than a collection or a counter type. CQL data types lists the types. Exceptions: ADD supports a collection type and also, if the table is a counter, a counter type.

A table must have a unique primary key whose first column must be of type DATETIME.
This column is used as partition key to distribute the rows among the hosts. 

- why partition key? immer die erste column von primary key, um rows auf hosts zu verteilen, optimierungen wenn von type time
for timeseries data it is strongly recommende to choose a time column as partition key

<br />

**NOTE:** If you are an EventQL Cloud user you can also create and update table
schemas from the web interface. Go to `EventQL Cloud > Tables` and click the
`Create Table` button on the top right corner.

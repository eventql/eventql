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


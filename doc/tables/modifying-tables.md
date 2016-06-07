2.1.4 Modyfing Tables
=====================

    ALTER TABLE table_name
    alter_specification [, alter_specification, ...]

    alter_specification ::=
        ADD [COLUMN] column_definition
      | DROP [COLUMN] column_name

    column_definition ::=
        column_name [REPEATED] cql_type [NOT NULL]


Alter table changes the structure of the table by adding or removing columns.


To alter the schema of a RECORD column, the subcolumns are specified by their name
that consits of the parent column name followed by a point followed by the actual
column name.

Add a record column with two subcolumns:

    ALTER TABLE evtbl
        ADD COLUMN product RECORD,
        ADD COLUMN product.id UINT64,
        ADD COLUMN product.title STRING;


If you want to learn more about how to alter the table structure using the HTTP API,
please refer to the [HTTP API reference](/documentation/api/http/).

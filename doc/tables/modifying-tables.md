3.4 Modifying Tables
===================

    ALTER TABLE table_name
    alter_specification [, alter_specification, ...]

    alter_specification ::=
        ADD [COLUMN] column_definition
      | DROP [COLUMN] column_name

    column_definition ::=
        column_name [REPEATED] sql_type [NOT NULL]


Alter table changes the structure of the table by adding or removing columns.

#### Example:

    ALTER TABLE temperature_measurements ADD COLUMN sensor_location STRING;

**NOTE:** If you are an EventQL Cloud user you can also create and update table
schemas from the web interface. Go to `EventQL Cloud > Tables` and click the
`Create Table` button on the top right corner.


### Altering RECORD columns

You can also alter the (sub-)schema of a RECORD column. To do so, simply specify
the column name as `parent1.parent2.parentN.column_name`.

Fore example:

    ALTER TABLE nested_table ADD COLUMN nested.something STRING;


### HTTP API

You can also alter tables using the HTTP API. Please refer to the [HTTP API reference](/documentation/api/http/).

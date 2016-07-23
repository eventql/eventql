ALTER TABLE statement
=====================

    ALTER TABLE table_name
    alter_specification [, alter_specification, ...]

    alter_specification ::=
        ADD [COLUMN] column_definition
      | DROP [COLUMN] column_name

    column_definition ::=
        column_name [REPEATED] cql_type [NOT NULL]



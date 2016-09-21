5.2.5 ALTER TABLE
=================

    ALTER TABLE table_name
    alter_specification [, alter_specification, ...]

    alter_specification ::=
        ADD [COLUMN] column_definition
      | DROP [COLUMN] column_name

    column_definition ::=
        column_name [REPEATED] sql_type [NOT NULL]


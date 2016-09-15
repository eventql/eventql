3.5 Deleting Tables
===================

    DROP TABLE table_name


It removes an existing table and all the table data.
Please note, this statement can only be performed if the configuration option
`cluster.allow_drop_table` is set to true.

#### Example:

    DROP TABLE sensors;


### HTTP API

You can also delete tables using the HTTP API. Please refer to the [HTTP API reference](/documentation/api/http/).


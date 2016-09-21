POST /api/v1/tables/create
================

Create a new EventQL table.<br>

A table must have a unique primary key whose first column is treated as
partition key to distribute the rows among the hosts. The partition key can be
of type `string`, `uint64` or `datetime`. 

**NOTE for high-volume timeseries:** If you are planning to store large volumes
timeseries-structured data in the table, please see the
[Timeseries & Logs page](../../../../collecting-data/high-volume-timeseries-logs) for
tips to get the best performance.

To learn more about primary keys and understand how to choose one to get the
best performance, read on the [Partitioning](../../../../tables/partitioning/) page.

###Resource Information
<table class='http_api create_table'>
  <tr>
    <td>Content-Type</td>
    <td>application/json</td>
  </tr>
</table>

###Parameters
<table class='http_api create_table'>
  <tr>
    <td>table_name</td>
    <td>The name of the table to be created.</td>
  </tr>
  <tr>
    <td>primary_key</td>
    <td>An array of column names that should be the primary key for this table</td>
  </tr>
  <tr>
    <td>database (optional)</td>
    <td>The name of the database.
  </tr>
  <tr>
    <td>columns.name</td>
    <td>The name of the column</td>
  </tr>
  <tr>
    <td>columns.type</td>
    <td>The SQL data type of the column</td>
  </tr>
  <tr>
    <td>columns.optional</td>
    <td>True if the column is optional, false otherwise</td>
  </tr>
  <tr>
    <td>columns.repeated</td>
    <td>True if the column is repeated, false otherwise</td>
  </tr>
  <tr>
    <td>columns.subcolumns</td>
    <td>The child columns of an OBJECT column.</td>
  </tr>
  <tr>
    <td>properties (optional)</td>
    <td>A list of key=value property pairs, encoded as an array of 2-element string arrays</td>
  </tr>
</table>

### Example Request

        >> POST /api/v1/tables/create_table HTTP/1.1
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "table_name": "my_sensor_table",
        >>   "primary_key": ["time", "sensor_name"],
        >>   "columns": [
        >>       {
        >>          "name": "time",
        >>          "type": "DATETIME"
        >>       },
        >>       {
        >>          "name": "sensor_name",
        >>          "type": "STRING"
        >>       },
        >>       {
        >>          "name": "sensor_value",
        >>          "type": "DOUBLE"
        >>       }
        >>   ],
        >>   "properties": [
        >>      [ "partition_size_hint", "300000000" ]
        >>   ]
        >> }


### Example Request of a table with nested columns

        >> POST /api/v1/tables/create_table HTTP/1.1
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "table_name": "my_sensor_table",
        >>   "primary_key": ["time", "sensor_name"],
        >>   "columns": [
        >>       {
        >>          "name": "time",
        >>          "type": "DATETIME"
        >>       },
        >>       {
        >>          "name": "sensor_name",
        >>          "type": "STRING"
        >>       },
        >>       {
        >>          "name": "measurements",
        >>          "type": "OBJECT",
        >>          "repeated": true,
        >>          "subcolumns": [
        >>              {
        >>                "name": "id",
        >>                "type": "STRING",
        >>              },
        >>              {
        >>                "name": "value",
        >>                "type": "DOUBLE",
        >>              }
        >>          ]
        >>        }
        >>   ],
        >>   "properties": [
        >>      [ "partition_size_hint", "300000000" ]
        >>   ]
        >> }

### Example Response

        << HTTP/1.1 201 CREATED
        << Content-Length: 0


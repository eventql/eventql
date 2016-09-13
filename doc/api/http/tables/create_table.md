POST /api/v1/tables/create_table
================

Create a new EventQL table.<br>

A table must have a unique primary key whose first column is treated as
partition key to distribute the rows among the hosts. The partition key can be
of type `string`, `uint64` or `datetime`. 

To learn more about primary keys and understand how to choose one to get the
best performance, read on the [Partitioning](../../../../tables/partitioning/) page.

###Resource Information
<table class='http_api create_table'>
  <tr>
    <td>Authentication required?</td>
    <td>Yes</td>
  </tr>
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
    <td>schema</td>
    <td>The json schema of the table.</td>
  </tr>
  <tr>
    <td>properties (optional)</td>
    <td>A list of key=value property pairs, encoded as an array of 2-element string arrays</td>
  </tr>
</table>


### Example Request

        >> POST /api/v1/tables/create_table HTTP/1.1
        >> Authorization: Token <authtoken>
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "table_name": "sensors",
        >>   "primary_key": ["time", "sensor_name"],
        >>   "schema": {
        >>      "columns": [
        >>          {
        >>             "id": 1,
        >>             "name": "time",
        >>             "type": "DATETIME"
        >>          },
        >>          {
        >>             "id": 2,
        >>             "name": "sensor_name",
        >>             "type": "STRING"
        >>          },
        >>          {
        >>             "id": 3,
        >>             "name": "sensor_value",
        >>             "type": "DOUBLE"
        >>          }
        >>      ]
        >>   },
        >>   "properties": [
        >>      [ "finite_partition_size", "300000000" ]
        >>   ]
        >> }

### Example Response

        << HTTP/1.1 201 CREATED
        << Content-Length: 0

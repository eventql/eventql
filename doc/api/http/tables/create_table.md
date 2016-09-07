POST /api/v1/tables/create_table
================

Create a new EventQL table or update the schema of an existing table.<br>
A table must have at least one unique primary key whose first column is treated as partition key to distribute the rows among the hosts. The partition key can be of type `string`, `uint64` or `datetime`. To learn more about primary keys and understand how to choose one to get the best performance, read on the [Partitioning](../../../../tables/partitioning/) page.

How to choose a good partition size? Currently it's best to aim for roughly
250MB-1Gper partition. If unsure, leave it with the default. In the future the
partition size will be automatically adjusted on the fly so you don't have to
choose one. Tables with a fixed partition size will automatically transition
to this new scheme.

Please make sure that each field has a unique numeric ID (it's basically
a protobuf schema).

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
    <td>database (optional)</td>
    <td>The name of the database. Optional if an Authentication header is sent>
  <tr>
    <td>table&#95;name</td>
    <td>The name of the table to be created/overwritten.</td>
  </tr>
  <tr>
    <td>primary_key</td>
    <td>An list of column names that are part of the primary key. Note, the first column is treated as partition key.</td>
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
</table>


###Example Request

        >> POST /api/v1/tables/create_table HTTP/1.1
        >> Authorization: Token <authtoken>
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "table_name": "my_sensor_table",
        >>   "columns": [
        >>       {
        >>          "name": "time",
        >>          "type": "DATETIME",
        >>          "optional": false,
        >>          "repeated": false
        >>       },
        >>       {
        >>          "name": "sensor_name",
        >>          "type": "STRING",
        >>          "optional": false,
        >>          "repeated": false
        >>       },
        >>       {
        >>          "name": "error",
        >>          "type": "RECORD",
        >>          "optional": true,
        >>          "repeated": false,
        >>          "columns": [
        >>            {
        >>              "name": "type",
        >>              "type": "STRING",
        >>              "optional": true,
        >>              "repeated": false,
        >>            },
        >>            {
        >>              "name": "messsage",
        >>              "type": "STRING",
        >>              "optional": true,
        >>              "repeated": false,
        >>            }
        >>          ]
        >>       }
        >>   ],
        >>   "primary_key": ["time", "sensor_name"]
        >> }


###Example Response

        << HTTP/1.1 201 CREATED
        << Content-Length: 0

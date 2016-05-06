POST /api/v1/tables/create_table
================

Create a new EventQL table or overwrite an existing table.

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
    <td>table&#95;name</td>
    <td>name of the table to be created/overwritten</td>
  </tr>
  <tr>
    <td>table&#95;type</td>
    <td>the type of the table to be created. must be "static" or "timeseries"</td>
  </tr>
  <tr>
    <td>schema</td>
    <td>the json schema of the table</td>
  </tr>
  <tr>
    <td>update (optional)</td>
    <td>if true, overwrite an existing table if a table with the same name exists</td>
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
        >>   "table_type": "timeseries",
        >>   "schema": {
        >>      "columns": [
        >>          {
        >>             "id": 1,
        >>             "name": "time",
        >>             "type": "DATETIME",
        >>             "optional": false,
        >>             "repeated": false
        >>          },
        >>          {
        >>             "id": 2,
        >>             "name": "sensor_name",
        >>             "type": "STRING",
        >>             "optional": false,
        >>             "repeated": false
        >>          },
        >>          {
        >>             "id": 3,
        >>             "name": "sensor_value",
        >>             "type": "DOUBLE",
        >>             "optional": true,
        >>             "repeated": false
        >>          }
        >>      ]
        >>   }
        >> }


###Example Response

        << HTTP/1.1 201 CREATED
        << Content-Length: 0

POST /api/v1/tables/describe
==========================

Fetch the column list of a table.

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
    <td>table</td>
    <td>The name of the table.</td>
  </tr>
  <tr>
    <td>database (optional)</td>
    <td>The name of the database.
  </tr>
</table>

###Example Request

        >> POST /api/v1/tables/describe HTTP/1.1
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "table": "my_sensor_table"
        >> }


###Example Response

        << HTTP/1.1 200 OK
        << Content-Type: application/json
        << Content-Length: ...
        <<
        << {
        <<  "name": "",
        <<  "columns": [
        <<    {
        <<     "id": 1,
        <<      "name": "sensor_name",
        <<      "type": "STRING",
        <<      "type_size": 0,
        <<      "optional": true,
        <<      "repeated": false,
        <<      "encoding_hint": "NONE"
        <<    }
        <<  ]
        << }

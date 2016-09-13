POST /api/v1/tables/describe
==========================

Fetch the definition of the table indicated by the table parameter.
The definition is returned as a JSON object including meta information about the table as well as a list of the columns.

###Resource Information
<table class='http_api create_table'>
  <tr>
    <td>Response Content-Type</td>
    <td>application/json</td>
  </tr>
</table>

###Parameters
<table class='http_api create_table'>
  <tr>
    <td>table</td>
    <td>Name of the table which definition should be retrieved.</td>
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

        << {
        <<   "table": {
        <<     "name": "my_sensor_table",
        <<       "tags": [],
        <<       "columns: [
        <<         {
        <<           "column_name": "sensor_name",
        <<           "type": "string",
        <<           "is_nullable": false
        <<         }
        <<       ]
        <<       "schema": {
        <<       "name": "",
        <<       "columns": [
        <<         {
        <<           "id": 1,
        <<           "name": "sensor_name",
        <<           "type": "STRING",
        <<           "type_size": 0,
        <<           "optional": true,
        <<           "repeated": false,
        <<           "encoding_hint": "NONE"
        <<         }
        <<       ]
        <<     }
        <<   }
        << }

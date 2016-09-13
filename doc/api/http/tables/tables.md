POST /api/v1/tables
==========================

Retrieve an ordered list of all existing tables.

###Resource Information
<table class='http_api'>
  <tr>
    <td>Request Content-Type</td>
    <td>application/json</td>
  </tr>
  <tr>
    <td>Response Content-Type</td>
    <td>application/json</td>
  </tr>
</table>

###Parameters
<table class='http_api'>
  <tr>
    <td>order</td>
    <td>How the list should be ordered. Must be "desc" or "asc". Defaults to "asc"</td>
  </tr>
</table>

###Example Request

        >> POST /api/v1/tables HTTP/1.1
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "database": "sensor_database",
        >>   "order": "desc"
        >> }


###Example Response

        << {
        <<   "tables": [
        <<     {
        <<       "name": "my_sensor_table",
        <<       "tags": []
        <<     }
        <<   ]
        << }

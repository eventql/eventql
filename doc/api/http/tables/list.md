POST /api/v1/tables/list
==========================

Retrieve an ordered list of all existing tables.

###Resource Information
<table class='http_api'>
  <tr>
    <td>Content-Type</td>
    <td>application/json</td>
  </tr>
</table>

###Parameters
<table class='http_api'>
  <tr>
    <td>database (optional)</td>
    <td>The name of the database.
  </tr>
  <tr>
    <td>order (optional)</td>
    <td>The order of the table names ("desc" or "asc"). Defaults to "asc".</td>
  </tr>
</table>

###Example Request

        >> POST /api/v1/tables/list HTTP/1.1
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "database": "sensor_database",
        >>   "order": "desc"
        >> }


###Example Response

        << HTTP/1.1 200 OK
        << Content-Type: application/json
        << Content-Length: ...
        <<
        << {
        <<   "tables": [
        <<     {
        <<       "name": "my_sensor_table"
        <<     }
        <<   ]
        << }

GET /api/v1/tables
==========================

Retrieve an ordered list of all existing tables.

###Resource Information
<table class='http_api'>
  <tr>
    <td>Authentication required?</td>
    <td>Yes</td>
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
    <td>Order of the table names. Must be "desc" or "asc". Defaults to "asc"</td>
  </tr>
</table>

###Example Request

        >> GET /api/v1/tables?order=desc HTTP/1.1
        >> Authorization: Token <authtoken> ...

###Example Response

        << {
        <<   "tables": [
        <<     {
        <<       "name": "my_sensor_table",
        <<       "tags": []
        <<     }
        <<   ]
        << }

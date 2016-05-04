GET /api/v1/sql
==========================

Execute SQL

###Resource Information
<table class='http_api'>
  <tr>
    <td>Authentication required?</td>
    <td>Yes</td>
  </tr>
</table>

###Parameters
<table class='http_api'>
  <tr>
    <td>format</td>
    <td>The response format. Must be one of "ascii", "binary", "json" or "json_sse". Defaults to "json"</td>
  </tr>
  <tr>
    <td>query</td>
    <td>The query string to execute.</td>
  </tr>
</table>

###Example Request

        >> GET /api/v1/sql?format=json&query=SELECT%20sensor_value%2C%20time%20FROM%20
           my_sensor_table%20WHERE%20sensor_name%20%3D%20%22temperature_outside%22%3B HTTP/1.1
        >> Authorization: Token <authtoken> ...


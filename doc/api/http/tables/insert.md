POST /api/v1/tables/insert
==========================

Insert rows into an existing table. The request should be a JSON
array containing a list of objects. Each object represents one row to be
inserted. You may therefore insert one or more rows per request.

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
    <td>database</td>
    <td>Name of the database.</td>
  </tr>
  <tr>
    <td>table</td>
    <td>Name of the table into which the row should be inserted.</td>
  </tr>
  <tr>
    <td>data</td>
    <td>JSON Object containing the actual data to be inserted.</td>
  </tr>
</table>

### Request Example

        >> POST /api/v1/tables/insert HTTP/1.1
        >> Authorization: Token <authtoken>
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> [
        >>   {
        >>     "database": "sensor_database",
        >>     "table": "my_sensor_table",
        >>     "data": {
        >>       "time": "2015-10-11T17:53:03Z",
        >>       "sensor_name": "temperature_outside",
        >>       "sensor_value": 8.634
        >>     }
        >>   },
        >>   {
        >>     "database": "sensor_database",
        >>     "table": "my_sensor_table",
        >>     "data": {
        >>       "time": "2015-10-11T17:53:04Z",
        >>       "sensor_name": "temperature_inside",
        >>       "sensor_value": 21.282
        >>     }
        >>   }
        >> ]


###Response Example

        << HTTP/1.1 201 CREATED
        << Content-Length: 0

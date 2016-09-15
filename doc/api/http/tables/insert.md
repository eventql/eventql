POST /api/v1/tables/insert
==========================

Insert one or more rows into an existing table. 

###Resource Information
<table class='http_api create_table'>
  <tr>
    <td>Request Content-Type</td>
    <td>application/json</td>
  </tr>
</table>

###Parameters
The body consist of one or more row object with the following properties:

<table class='http_api create_table'>
  <tr>
    <td>database</td>
    <td>Name of the database.</td>
  </tr>
  <tr>
    <td>table</td>
    <td>Name of the table to insert the row to.</td>
  </tr>
  <tr>
    <td>data</td>
    <td>JSON Object containing the actual data to be inserted.</td>
  </tr>
</table>

### Example Request

        >> POST /api/v1/tables/insert HTTP/1.1
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


### Example Response

        << HTTP/1.1 201 CREATED
        << Content-Length: 0

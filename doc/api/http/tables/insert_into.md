POST /api/v1/tables/insert_into
===============================

Insert one or more rows into an existing table. 

###Resource Information
<table class='http_api create_table'>
  <tr>
    <td>Request Content-Type</td>
    <td>application/json</td>
  </tr>
</table>

###URL Paramters
<table class='http_api create_table'>
  <tr>
    <td>database</td>
    <td>Name of the database.</td>
  </tr>
  <tr>
    <td>table</td>
    <td>Name of the table to insert the row to.</td>
  </tr>
</table>

###JSON Body
The body consists of one or more objects containing the data to be inserted.

### Example Request

        >> POST /api/v1/tables/insert_into?database=sensors&table=tmp HTTP/1.1
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> [
        >>   {
        >>     "time": "2015-10-11T17:53:03Z",
        >>     "sensor_name": "temperature_outside",
        >>     "sensor_value": 8.634
        >>   },
        >>   {
        >>     "time": "2015-10-11T17:53:04Z",
        >>     "sensor_name": "temperature_inside",
        >>     "sensor_value": 21.282
        >>   }
        >> ]


### Example Response

        << HTTP/1.1 201 CREATED
        << Content-Length: 0


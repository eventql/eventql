GET /api/v1/sql
=======================

Perform a query against the EventQL database. The query string to execute can contain multiple
queries that should each end with a semicolon.




EventQL provides different formats in which the result can be returned.
if you want to stream constant  updates about the status of the query please use "json_sse"

  json_sse provides constant updates ...

When using "json_sse",

Updates about the status of the query execution will be send

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
    <td>The sql statement. It should end with a semicolon.</td>
  </tr>
</table>

###Using Server-Sent Events
When using the format "json-sse" an EventSource (or Server-Sent Events) Stream is
created and events will be repeatedly sent to the client. Updates about te

an event always includes the properties type and data. 

The follwing events are available:

Status Event (type = "status") is sent ....
<table>
  <tr>
    <td>status</td>
    <td></td>
  </tr>
  <tr>
    <td>message</td>
    <td>Human one of running, waiting, downloading</td>
  </tr>
  <tr>
    <td>progress</td>
    <td></td>
  </tr>
</table>

The result event (type = "result") sends the actual query result in the json response format described below.


Query Error Event (type = "query_error") is sent when an error was thrown while executing the query.
<table>
  <tr>
    <td>error</td>
    <td>The error message returned by the SQL engine.</td>
  </tr>
</table>

Error Event (type = "error") ... No data will be sent for this event.



###Example Request with format=json

        >> GET /api/v1/sql?format=json&query=SELECT%20sensor_name%2C%20
           sensor_value%2C%20time%20FROM%20my_sensor_table%3B HTTP/1.1
        >> Authorization: Token <authtoken> ...



###Example Response

        << {
        <<   "results": [
        <<     {
        <<       "type": "table",
        <<       "columns": ["sensor_name", "sensor_value", "time"],
        <<       "rows": [
        <<         ["temperature_outside", 8.634, "2015-10-11T17:53:03Z"],
        <<         ["temperature_inside", 21.282, "2015-10-11T17:53:04Z"]
        <<       ]
        <<     }
        <<   ]
        << }

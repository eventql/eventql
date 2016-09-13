POST /api/v1/sql
=======================

Perform a query against the EventQL database. The query string to execute can 
contain multiple queries that should each end with a semicolon.


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
    <td>query</td>
    <td>The sql statement. It should end with a semicolon.</td>
  </tr>
  <tr>
    <td>format (optional)</td>
    <td>The response format ("json" or "json_sse"). Defaults to "json".</td>
  </tr>
  <tr>
    <td>database (optional)</td>
    <td>The name of the database.
  </tr>
</table>

##### Example

        >> POST /api/v1/tables/create_table HTTP/1.1
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "query": "SELECT * from my_sensor_table",
        >>   "format": "json"
        >> }

        << HTTP/1.1 200 OK
        << Content-Type: application/json
        << Content-Length: ...
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


#### json_sse

To obtain continuous query status updates, EventQL supports Server-Sent Events.
Three different event types can be sent: `status`, `result` and `error`.

##### Example

        >> POST /api/v1/tables/create_table HTTP/1.1
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "query": "SELECT * from my_sensor_table",
        >>   "format": "json_sse"
        >> }

        << HTTP/1.1 200 OK
        << Content-Type: text/event-stream
        <<
        << event: status
        << data: {"status": "running","progress": 0.0,"message": "Waiting..."}
        <<
        << event: status
        << data: {"status": "running","progress": 0.5,"message": "Running..."}
        <<
        << event: result
        << data: {
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


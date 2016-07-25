GET /api/v1/sql
=======================

Perform a query against the EventQL database. The query string to execute can contain multiple
queries that should each end with a semicolon.


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
    <td>The response format. Must be"json" or "json_sse". Defaults to "json"</td>
  </tr>
  <tr>
    <td>query</td>
    <td>The sql statement. It should end with a semicolon.</td>
  </tr>
</table>

### Result Formats
The query result is sent as JSON string that contains an array of results. 
Every result set is made up of a list of columns and a list of lists of values.


##### Example

        >> GET /api/v1/sql?format=json&query=SELECT%20sensor_name%2C%20
           sensor_value%2C%20time%20FROM%20my_sensor_table%3B HTTP/1.1
        >> Authorization: Token <authtoken> ...

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
To obtain continuous query status updates, EventQL supports Server-Sent Events, that is an event stream sent over HTTP.
The client just listens to that stream and receives updates as well as the acutal query result or error messages from the EventQL server.

Events with the follwing types are sent by EventQL:

The `status` event provides information about the current status of the query execution. <br>
It has the properties `status`, , `message`, a description of the current status that is one of Waiting, Running and Downloading, and `progress`, the status in percent.

The `result` event provides the acutal result JSON string.

An `error` is sent if somethin goes wrong during the query execution. It contains the error message returned by the EventQL server.

##### Example using Javascript

    var query = new EventSource(".../api/v1/sql?format=json_sse&query=SELECT%20sensor_name%2C%20sensor_value%2C%20time%20FROM%20my_sensor_table%3B");

    query.addEventListener("result", function(e) {
      query.close();
      var results = JSON.parse(e.data).results;
    });

    query.addEventListener("status", function(e) {
      var progress = JSON.parse(e.data).progress;
    });

    query.addEventListener("error", function(e) {
      var error = e.data;
      query.close();
    });
    

POST /api/v1/tables/drop
==========================

Remove an existing table. Please note, this statement can only be performed
if the configuration option `cluster.allow_drop_table` is set true.

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
    <td>table</td>
    <td>The name of the table to remove.</td>
  </tr>
  <tr>
    <td>database (optional)</td>
    <td>The name of the database.</td>
  </tr>
</table>

###Example Request

        >> POST /api/v1/tables/drop HTTP/1.1
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "table": "sensors"
        >> }


###Example Response

        << HTTP/1.1 201 OK
        << Content-Type: application/json
        << Content-Length: ...


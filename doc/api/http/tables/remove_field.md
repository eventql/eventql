POST /api/v1/tables/remove_field
================

Remove an existing field from an existing table.
To alter the (sub-)schema of a RECORD column, specify the column name as `parent1.parent2.parentN.column_name`.

Please note, that fields that are part of the primary key can't be deleted.


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
    <td>The name of the table to remove the field from.</td>
  </tr>
  <tr>
    <td>field&#95;name</td>
    <td>The name of the field to remove.</td>
  </tr>
  <tr>
    <td>database (optional)</td>
    <td>The name of the database.
  </tr>
</table>

###Example Request

        >> POST /api/v1/tables/create_table HTTP/1.1
        >> Authorization: Token <authtoken>
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "table_name": "my_sensor_table",
        >>   "field_name": "location"
        >> }


###Example Response

        << HTTP/1.1 201 Created
        << Content-Length: 0

POST /api/v1/tables/remove_field
================

Remove an existing field from an existing table.
To alter the (sub-)schema of a RECORD column, specify the column name as `parent1.parent2.parentN.column_name`.

Please note, that fields that are part of the primary key can't be deleted.


###Resource Information
<table class='http_api'>
  <tr>
    <td>Authentication required?</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>Content-Type</td>
    <td>text/plain</td>
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
</table>

###Example Request

        >> POST /api/v1/tables/remove_field?table=my_sensor_table&field_name=sensor_location HTTP/1.1
        >> Authorization: Token <authtoken>
        >> Content-Type: text/plain
        >> Content-Length: ...


###Example Response

        << HTTP/1.1 201 Created
        << Content-Length: 0

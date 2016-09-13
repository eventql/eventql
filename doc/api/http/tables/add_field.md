POST /api/v1/tables/add_field
================

Add a new field to an existing table.

###Resource Information
<table class='http_api'>
  <tr>
    <td>Content-Type</td>
    <td>application/json</td>
  </tr>
</table>


###Parameters:
<table class='http_api'>
  <tr>
    <td>database (optional)</td>
    <td>The name of the database.
  </tr>
  <tr>
    <td>table</td>
    <td>The name of an existing table to add the field to.</td>
  </tr>
  <tr>
    <td>field&#95;name</td>
    <td>The name of the field to add.</td>
  </tr>
  <tr>
    <td>field&#95;type</td>
    <td>The data type of the new field. Any EventQL data type is allowed.</td>
  </tr>
  <tr>
    <td>repeated</td>
    <td>Boolean. If true, the field type is specified as repeated.</td>
  </tr>
  <tr>
    <td>optional</td>
    <td>Boolean. If true, the field is marked as optional.</td>
  </tr>
</table>

###Example Request

        >> POST /api/v1/tables/add_field HTTP/1.1
        >> Authorization: Token <authtoken>
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "table": "my_sensor",
        >>   "field_name": "location",
        >>   "field_type": "STRING",
        >>   "repeated": false,
        >>   "optional": true
        >> }


###Example Response

        << HTTP/1.1 201 Created
        << Content-Length: 0

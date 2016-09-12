POST /api/v1/tables/add_field
================

Add a new field with specified data type to an existing table. <br>
For more information on the data types supported by EventQL, please refer to
<a href="/documentation/tables/datatypes">Chapter 2.1.1 </a>

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


###Parameters:
<table class='http_api'>
  <tr>
    <td>table</td>
    <td>The name of an existing table to add the field to.</td>
  </tr>
  <tr>
    <td>field&#95;name</td>
    <td>The name for the new field.</td>
  </tr>
  <tr>
    <td>field&#95;type</td>
    <td>Data type of the new field. Any EventQL data type is allowed.</td>
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

        >> POST /api/v1/tables/add_field?table=my_sensor_table&field_name=sensor_location&
           field_type=STRING&repeated=false&optional=true HTTP/1.1
        >> Authorization: Token <authtoken>
        >> Content-Type: text/plain
        >> Content-Length: ...


###Example Response

        << HTTP/1.1 201 Created
        << Content-Length: 0

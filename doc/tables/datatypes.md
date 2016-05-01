2.1.1 Data Types
================

EventQL supports a number of SQL data types in several categories: numeric types,
date and time types and string types. This page provides an overview of these data
types and a summary of the data type storage requirements. 

#### Data Types

<table class="table">
  <tr>
    <td>STRING<td>
    <td><td>
    <td><td>
  </tr>
  <tr>
    <td>DOUBLE<td>
    <td><td>
    <td><td>
  </tr>
  <tr>
    <td>UINT32<td>
    <td><td>
    <td><td>
  </tr>
  <tr>
    <td>UINT64<td>
    <td><td>
    <td><td>
  </tr>
  <tr>
    <td>BOOLEAN<td>
    <td><td>
    <td><td>
  </tr>
  <tr>
    <td>DATETIME<td>
    <td><td>
    <td><td>
  </tr>
  <tr>
    <td>OBJECT<td>
    <td><td>
    <td><td>
  </tr>
</table>


#### Repeated Fields

EventQL schemas allow for a type to be specified as `repeated`. A repeated field
can be repeated any number of times (including zero) in a well-formed row.
The order of the repeated values will be preserved.

Let's look at a simple example:

    schema ev.measurements {
      datetime measurement_time;
      string sensor_id;
      repeated string measured_values;
    }

And an example JSON message that is valid for the above schema:

    {
      "measurement_time": ...,
      "sensor_id": ...,
      "measured_values": [ "A", "B", "C"]
    }


#### The OBJECT Type

The object type allows you to define nesting within column. Its most useful in
combination with repeated fields as this allows you to build schemas that can
properly represent arbitrary JSON objects.

We can modify our example above to include a location with each measure value:

    schema ev.measurements {
      datetime measurement_time;
      string sensor_id;
      repeated object measured_values {
        string value;
        string location;
      }
    }


This JSON message can now be stored:

    {
      "measurement_time": ...,
      "sensor_id": ...,
      "measured_values": [
        { "value": "A", "location": "loc1" },
        { "value": "B", "location": "loc2" },
        { "value": "C", "location": "loc3" }
      ]
    }


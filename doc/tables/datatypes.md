3.1 Data Types
==============

EventQL supports a number of SQL data types in several categories: numeric types,
date and time types and string types. This page provides an overview of these data
types and a summary of the data type storage requirements. 

#### Data Types

<table class="table">
  <tr>
    <th>Type</th>
    <th>Min Value</th>
    <th>Max Value</th>
    <th>Size (Bits)</th>
  </tr>
  <tr>
    <td>STRING</td>
    <td>&mdash;</td>
    <td>&mdash;</td>
    <td>8-32 + $len * 8</td>
  </tr>
  <tr>
    <td>DOUBLE</td>
    <td>-1.7976931348623157E+308</td>
    <td>1.7976931348623157E+308</td>
    <td>64</td>
  </tr>
  <tr>
    <td>UINT32</td>
    <td>0</td>
    <td>4294967295</td>
    <td>8-32</td>
  </tr>
  <tr>
    <td>UINT64</td>
    <td>0</td>
    <td>18446744073709551615</td>
    <td>8-64</td>
  </tr>
  <tr>
    <td>BOOLEAN</td>
    <td>0</td>
    <td>1</td>
    <td>1</td>
  </tr>
  <tr>
    <td>DATETIME</td>
    <td>0</td>
    <td>18446744073709551615</td>
    <td>8-64</td>
  </tr>
  <tr>
    <td>RECORD</td>
    <td>&mdash;</td>
    <td>&mdash;</td>
    <td>0</td>
  </tr>
</table>


### Repeated Fields

EventQL schemas allow for a type to be specified as `repeated`. A repeated field
can be repeated any number of times (including zero) in a well-formed row.
The order of the repeated values will be preserved.

Let's look at a simple example:

    CREATE TABLE ev.temperature_measurements (
      collected_at    DATETIME,
      sensor_id       STRING,
      measured_values REPEATED STRING,
      PRIMARY KEY(collected_at, sensor_id)
    );

And an example JSON message that is valid for the above schema:

    {
      "collected_at": "2016-07-05 13:34:51",
      "sensor_id": "sensor1",
      "measured_values": [ "A", "B", "C"]
    }

### The RECORD Type

The record type allows you to define nesting within column. Its most useful in
combination with repeated fields as this allows you to build schemas that can
properly represent arbitrary JSON objects.

We can modify our example above to include a location with each measure value:

    CREATE TABLE ev.temperature_measurements (
      collected_at DATETIME,
      sensor_id    STRING,
      measurements REPEATED RECORD (
        thing_id     STRING,
        temperature  DOUBLE
      ),
      PRIMARY KEY(collected_at, sensor_id)
    );

This JSON message can now be stored:

    {
      "collected_at": "2016-07-05 13:34:51",
      "sensor_id": "sensor1",
      "measured_values": [
        { "thing_id": "thing1", temperature: 22.3 },
        { "thing_id": "thing2", temperature: 21.8 },
        { "thing_id": "thing3", temperature: 24.5 },
      ]
    }


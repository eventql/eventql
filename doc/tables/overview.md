2.1 Tables & Schemas
====================

The core unit of data storage in EventQL are tables and rows (also referred to
as records or events). Tables have a strict schema that you must define and that
all rows must adhere to.

Schema changes are instant since they only require a small metadata change on disk.


## Flat Records

The most simple form of a table schema is one that has a flat list of columns
with a simple data type like `string`, `uint64` or `datetime`. These table
schemas look exactly like the ones you are probably used to from other databases.
In fact, EventQL can import tables from all major relational database products.

Let's look at a simple example. Say we wanted to collect data
from a number of temperature sensors. Our table schema could look something like
this:

    schema ev.temperature_measurements {
      datetime  collected_at;
      string    sensor_id;
      double    temperature;
    }

Now we can insert some data and then query it. Something along these lines:

    evql> SELECT * from ev.temperature_measurements;

    ==================================================
    | collected_at         | sensor_id | temperature |
    ==================================================
    | 2015-02-05 13:34:51  | t3        | 20.3        |
    | 2015-02-05 13:34:53  | t1        | 21.2        |
    | 2015-02-05 13:34:56  | t2        | 21.7        |
    ==================================================


EventQL also accepts and returns rows as JSON or Protobuf. You'll see why
this is useful once we discuss nested records. These are the same rows as above
but in the JSON representation:

    { "collected_at": "2015-02-05 13:34:51", "sensor_id": "t3", "temperature": 20.3 }
    { "collected_at": "2015-02-05 13:34:53", "sensor_id": "t1", "temperature": 21.2 }
    { "collected_at": "2015-02-05 13:34:56", "sensor_id": "t2", "temperature": 21.7 }


## Nested Records

Alas, some data doesn't fit into this simple, flat model. Imagine for example that
you want to store a table of xxx where each xxx has multiple yyy. You can't fit
that into a flat schema (short of resorting to hacks like storing serialized
messags in string columns).

The concept is best illustrateted by example: Imagine we still want to store our
table of xxxs. This time we create a table with one column, yyy, that is repeated
and has the type "OBJECT". This 'yyy' column then has two sub-columns called
aaa and bbb.

    schema temperature_sensor_data {
      datetime collected_at;
      string   sensor_id;
      double   temperature;
    }

Now is the time to shine for our JSON representation. Here are some example
rows that we can store in the table:

    { "collected_at": "2015-02-05 13:34:51", "sensor_id": "t3", "temperature": 20.3 }
    { "collected_at": "2015-02-05 13:34:53", "sensor_id": "t1", "temperature": 21.2 }
    { "collected_at": "2015-02-05 13:34:56", "sensor_id": "t2", "temperature": 21.7 }

In general, nested rows allow you to store arbitrarily complex data structures
in a table row and still maintain a strict schema. You can insert nested rows as
JSON objects or via SQL. Tables with nested rows can be queried like any other
tables using either SQL or the JavaScript Query framework.

For exmaple, lets' say we wante to calculate the .... This is how we could do it

    SQL example

At any rate you won't need to understand the nested row feature until you want to
use it. Check out the ["Nested Records" page](/docs/sql/nested_records) for more details.



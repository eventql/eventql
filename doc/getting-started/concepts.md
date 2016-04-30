1.2 Concepts
============

From a user perspective, interacting with an EventQL instance feels similar to
interacting with most traditional SQL databases. Data is stored as rows in
schemaful tables. You can insert and retrieve rows, execute complex queries and
create, edit and drop tables.

However, EventQL is not a general purpose database. It was designed for large scala
data analysis and processing and has a fully distributed architecture. This
design allows you to ingest, process and query massive amounts of data at low
latency, but also makes EventQL less suited for classical transaction processing tasks.

This page explains the major concepts:

  - [Tables](#tables)
  - [SQL and JavaScript Queries](#sql-and-javascript-queries)
  - [Metrics](#metrics)
  - [Plugins](#plugins)

### Tables

The core unit of data storage in EventQL are tables and rows. Tables have a strict
schema that you must define and table columns are strictly typed.

The most simple form of a table schema is one that has a flat list of columns
with a simple data type like `string`, `uint64` or `datetime`. These table
schemas look exactly like the ones you are probably used to from other databases.
In fact, EventQL can import tables from all major database products with a few clicks.

  Schema changes are instant since they only require a small metadata change on disk.

Example: A simple table schema that stores data collected by a number of
temperature sensors.

      Schema:

        table temperature_sensor_data {
          datetime collected_at;
          string   sensor_id;
          double   temperature;
        }

      Example Rows:

        ==================================================
        | collected_at         | sensor_id | temperature |
        ==================================================
        | 2015-02-05 13:34:51  | t3        | 20.3        |
        | 2015-02-05 13:34:53  | t1        | 21.2        |
        | 2015-02-05 13:34:56  | t2        | 21.7        |
        ==================================================

      Example Rows (JSON representation)

        { "collected_at": "2015-02-05 13:34:51", "sensor_id": "t3", "temperature": 20.3 }
        { "collected_at": "2015-02-05 13:34:53", "sensor_id": "t1", "temperature": 21.2 }
        { "collected_at": "2015-02-05 13:34:56", "sensor_id": "t2", "temperature": 21.7 }

<br />
###### Nested Rows

Alas, some data doesn't fit into this simple, flat model. Imagine for example that
you want to store a table of xxx where each xxx has multiple yyy. You can't fit
that into a flat schema (short of resorting to hacks). Usually this is worked around
by splitting up the record into multiple tables and using relation ids to connect
rows in multiple tables (sometimes called normalization in RDBMS slang).

Anyhow, this approach has some drawbacks. For once you usually start
out with some form of structured data in the first place (like a JSON object)
and must somehow transform this record into a number of individual rows that
you then insert into multiple tables. Only to later re-assemble it at query time
using JOINs. This often adds unnessecary complexity. Moreover JOINs have inherent
performance and scalability issues.

While EventQL still alows you to import, create and use tables using the relational
paradigm it additionally supports nested and repeated columns. This may sound
complex but is actually quite simple and allows you to implement complex data
models traditionally not suited for SQL databases.

The concept is best explained by example: Imagine we still want to store our
table of xxxs. This time we create a table with one column, yyy, that is repeated
and has the type "OBJECT" this column then has two sub-columns called aaa and
bbb.


    Schema:

        table temperature_sensor_data {
          datetime collected_at;
          string   sensor_id;
          double   temperature;
        }

      Example Rows:

        ==================================================
        | collected_at         | sensor_id | temperature |
        ==================================================
        | 2015-02-05 13:34:51  | t3        | 20.3        |
        | 2015-02-05 13:34:53  | t1        | 21.2        |
        | 2015-02-05 13:34:56  | t2        | 21.7        |
        ==================================================

      Example Rows (JSON representation)

        { "collected_at": "2015-02-05 13:34:51", "sensor_id": "t3", "temperature": 20.3 }
        { "collected_at": "2015-02-05 13:34:53", "sensor_id": "t1", "temperature": 21.2 }
        { "collected_at": "2015-02-05 13:34:56", "sensor_id": "t2", "temperature": 21.7 }

In general, nested rows allow you to store arbitrarily complex data structures
in a table row and still maintain a strict schema. You can insert nested rows as
JSON objects or via SQL. Tables with nested rows can be queried like any other
tables using either SQL or the JavaScript Query framework.

At any rate you won't need to understand the nested row feature until you want to
use it. Check out the ["Nested Records" page](/docs/sql/nested_records) for more details.

<br />
###### Automatic Sharding &amp; Replication

Table storage is distributed across a cluster of physical servers and scales
linearly in the number of servers. Given enough servers, or in an EventQL Cloud
instance, you can store extremely large tables (>1000TB).

Queries are also automatically parallelized and executed on multiple physical
servers. The query planner considers data locality and always places the query
execution close to the data. This scheme usually results in a query speed-up
linear to the number of servers: Most queries will run 10x as fast on ten servers
they run on one server.

Updates don't have read-after-write consistency, but strong eventual consistency.

The sharding across nodes and rebalances are fully automatic and transparent.
This means you will never have to worry or think about it.

Still, you can optionally control the "replication factor" of any table. The
replication factor is an integral number of copies of each piece of data that
should be kept in the cluster. The default replication factor is 3, i.e. there
are 3 copies of every piece of data for redundancy and performance.


[Learn more about replication and sharding](/docs/internals/sharding_and_replication)

<br />
###### Columnar Storage

EventQL stores all data on disk in a columnar format: when executing a query, it
doesn't have to read the full row from disk (like row-oriented databases) but
only those columns which are actually required by the query. This feature is
fully transparent to the user: The only thing you will notice is dramatically
increased IO performance and therefore query execution speed.

Since queries don't have to read the full row from disk every time, you don't
have to limit yourself in terms of table size. EventQL can handle tables containing
many thousand individual columns per row just fine.


<br />
### SQL and JavaScript Queries

In addition to the full SQL query language, EventQL can execute JavaScript queries
and data processing pipelines allowing you to build the most complex data driven
applications.

###### Parallel Execution


###### ChartSQL Extensions

###### Nested Row Extensions

###### Batch and Stream Processing


### Metrics


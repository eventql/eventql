Getting Started with Z1
=======================

##### Getting Started in 5 minutes

We have 5 minute getting started guides for impatient people who don't like
reading documentation. For more in-depth information you are
kindly referred to the remaining chapters.

  + [SQL &mdash; 5 minute introduction](/docs/sql/introduction)
  + [MapReduce &mdash; 5 minute introduction](/docs/mapreduce/introduction)

##### Importing data

If you are just interested in sending data to Z1, read on:

  + [Web Tracking](/docs/connectors/web_tracking)
  + [iOS Tracking](/docs/connectors/ios_sdk)
  + [Android Tracking](/docs/connectors/android_sdk)
  + [Logfile Import](/docs/connectors/logfile_import)
  + [StatsD Import](/docs/connectors/statsd_import)

---

What is Z1?
-----------

Z1 is a distributed, analytical database. It allows you to store massive amounts
of structured data and explore it using SQL and powerful built-in query tools.
Additionally, you can run stream and batch data processing pipelines using a
simple JavaScript SDK to create advanced analytics or data-driven automations.

If you want to take the deep dive, start by reading [the "Concepts" page](/docs/concepts).

##### Distributed Queries &amp; Storage

Table storage and query execution is distributed across a cluster of physical server,
allowing you to store, process and query massive tables with trillions of records.
Sharding and replication is fully automatic and transparent. Queries are automatically
parallelized and distributed across servers.

##### Column-oriented

Z1 is a column-oriented database. This means that when executing a query, it
doesn't have to read the full row from disk (like row-oriented databases) but
only those columns which are actually required by the query. This feature results
in a dramatic speed-up for most queries but is otherwise transparent to the user.

##### Nested Records

The core unit of data storage in Z1 are tables and rows. Tables have a strict
schema and are strictly typed. Tables in Z1 may have "nested" schema that allow
you to store complex data structures (like a JSON object) in a single field.

##### SQL and JavaScript Queries

In addition to the full SQL query language, Z1 can execute JavaScript queries
and data processing pipelines allowing you to build the most complex data driven
applications.

##### Data Connectors

##### Query Tools & Report Editor

Z1 features a web UI that allows you to create and share complex reports
and dashboards using SQL, JavaScript and built-in graphical analytics and
query tools.

##### Plugins

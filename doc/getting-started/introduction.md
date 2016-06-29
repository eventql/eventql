1.1 Introduction
================

EventQL is a distributed, analytical database. It allows you to store massive amounts
of structured data and explore it using SQL and other programmatic query facilities.

From a user perspective, interacting with an EventQL instance feels similar to
interacting with most traditional SQL databases. Data is stored as rows in
schemaful tables. You can insert and retrieve rows, execute complex queries and
create, edit and drop tables.

However, EventQL is not a general purpose database. It was designed for large scala
data analysis and processing and has a fully distributed architecture. This
design allows you to ingest, process and query massive amounts of data at low
latency, but also makes EventQL less suited for classical transaction processing tasks.

## Getting Started with EventQL

**If you don't like reading documentation, start with the ["First Steps"](../first-steps/) page.** It
will walk you through starting a server, inserting events and running a query
in 10 minutes.

To take the deep dive, these pages give a good overview of the the major concepts:

  - [Concepts](../concepts/)
  - [Tables & Schemas](../../tables/)
  - [Partitioning](../../tables/partitioning/)
  - [Replication](../../tables/replication/)
  - [The SQL Query Language](../../sql/)
  - [JavaScript Queries](../../queries/)

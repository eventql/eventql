1.2 Concepts
============

This page gives a brief overview of the major concepts in EventQL with links to
detailed information in the respective chapters.

### Columnar Storage

EventQL stores all data on disk in a columnar format: when executing a query, it
doesn't have to read the full row from disk (like row-oriented databases) but
only those columns which are actually required by the query. This feature is
fully transparent to the user: The only thing you will notice is dramatically
increased IO performance and therefore query execution speed.

Since queries don't have to read the full row from disk every time, you don't
have to limit yourself in terms of table size. EventQL can handle tables containing
many thousand individual columns per row just fine.


### SQL and JavaScript Queries

In addition to the full SQL query language, EventQL can execute JavaScript queries
and data processing pipelines allowing you to build the most complex data driven
applications.

### Distributed Queries &amp; Storage

Table storage and query execution is distributed across a cluster of physical server,
allowing you to store, process and query massive tables with trillions of records.
Sharding and replication is fully automatic and transparent. Queries are automatically
parallelized and distributed across servers.

### Column-oriented

EventQL is a column-oriented database. This means that when executing a query, it
doesn't have to read the full row from disk (like row-oriented databases) but
only those columns which are actually required by the query. This feature results
in a dramatic speed-up for most queries but is otherwise transparent to the user.

### Nested Records

EventQL supports nested and repeated columns. This may sound complex but is actually
quite simple and allows you to implement complex data
models traditionally not suited for SQL databases.

The core unit of data storage in EventQL are tables and rows. Tables have a strict
schema and are strictly typed. Tables in EventQL may have "nested" schema that allow
you to store complex data structures (like a JSON object) in a single field.

### SQL and JavaScript Queries

In addition to the full SQL query language, EventQL can execute JavaScript queries
and data processing pipelines allowing you to build the most complex data driven
applications.


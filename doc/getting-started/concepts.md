1.3 Concepts
============

This page gives a brief overview of the major concepts in EventQL with links to
detailed information in the respective chapters.

### Tables & Schemas

The core unit of data storage in EventQL are tables and rows (also referred to
as rows or events). Tables have a strict schema that you must define and that
all rows must adhere to. You can currently insert rows as either JSON or
Protobuf messages

#### Columnar Storage Engine

EventQL is a column-oriented database. This means that when executing a query, it
doesn't have to read the full row from disk (like row-oriented databases) but
only those columns which are actually required by the query. This feature is
fully transparent to the user: The only thing you will notice is dramatically
increased IO performance and therefore query execution speed.

Since queries don't have to read the full row from disk every time, you don't
have to limit yourself in terms of table size. EventQL can handle tables containing
many thousand individual columns per row just fine.

EventQL implements the Dremel Record Shredding and Assembly algorithm to store
nested rows in columnar format.


#### Nested Records

Tables can have "nested" schemas via the OBJECT and REPEATED types. This may
sound complex but is actually quite simple and allows you to implement 
arbitrary data models traditionally not suited for SQL databases, such as JSON
events containing arrays or objects.

The nested rows concept is best [illustrated by example](../../tables/datatypes/).


#### Partitioning, Replication & Consistency

Tables are internally split into many partitions that can be distributed
over many machines and queried in parallel. This concept is sometimes referred to
as "massively parallel database architecture".

Available storage and query performance scales linearly ("horizontally") in the number of servers.
Given enough servers, or in an EventQL Cloud instance, you can store extremely
large tables (>1000TB). Partitioning and replication are fully automatic and transparent.

Updates don't have read-after-write consistency, but strong eventual consistency.


### SQL and JavaScript Queries

In addition to the full SQL query language, EventQL can execute JavaScript queries
and data processing pipelines allowing you to build the most complex data driven
applications.

#### Full SQL Support

EventQL already supports a large subset of standard SQL and continues to implement
more incrementally. All the standard stuff that you would expect works right now,
including JOINs.

#### Distributed Query Scheduler

Queries are also automatically parallelized and executed on multiple physical servers. The query planner considers data locality and always places the query execution close to the data. This scheme usually results in a query speed-up linear to the number of servers: Most queries will run 10x as fast on ten servers as they run on one server.


#### MapReduce &mdash; Batch and Stream Processing

To enable the most complex queries and data processing pipelines, EventQL allows you
to execute batch and stream processing jobs written in JavaScript.


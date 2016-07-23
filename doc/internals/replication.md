8.2 Replication
===============

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


### Consistency & Durability

Please note that if the write-ahead-log is turned off, which is the default, you can loose up to a few seconds of data if a node crashes. Updates/inserts are eventually consistent and have fully automatic conflict resolution (newest write wins with microsecond granularity).


8.1 Architecture
================

This page starts by giving a brief "10,000 feet" overview of the general
architecture of EventQL and then gives links to more detailed information about
the respective subsystems.

The primary audience for this page are EventQL developers and users that want to
learn about the internals of EventQL.

### Data Model

EventQL is a distributed database by design and is deployed onto a number of
machines called a "cluster". A cluster consists of many, equally privileged,
EventQL server instances that connect to a coordination service, like ZooKeeper.

The core unit of data storage in EventQL are tables and rows. On the data, it
supports the usual database operations, i.e. inserting, updating and querying
rows.

Each table has a strict schema and a mandatory primary key. This primary key is
used to automatically split a table into many partitions of roughly 500MB.
Each partition is then stored on N servers in the cluster.

Partioning is fully transparent to the user -- from a user perspective
interacting with an EventQL cluster feels just like interacting with an ordinary
SQL database.

Each copy of a partition is stored on the respective server's disk as a log
structured merge tree of segment files. Each segment file is encoded using the
cstable file format. CSTable is a container file format that stores many rows
of a given schema in column-oriented layout.

### Queries

Clients connect to an EventQL cluster to create and manage tables, insert and
update rows into tables and execute queries. When a client wants to executes a
SQL (or MapReduce) query on the data it can send the query to any server in the
cluster.

To execute a query, a server will first identify all tables referenced from the
query and then, for each table, identify the partitions that need to be scanned
to answer the query (taking into account WHERE restrictions on the primary key).
It will then rewrite the query into shards (in a simple query, one shard per
partition) so that as much work as possible can be performed local to the data. 
Then, all shards of the query are executed in parallel on the servers
that store the respective partitions, minimizing data transfers. On each shard,
the subset of the data that is required to answer the query (i.e. only the
actually referenced columns in the table) is loaded from the cstables on disk.
Finally, all shard results are merged and sent back to the client.

### Consistency & Durability

EventQL is _not_ intendend for transactional workloads but still gives some
consistency guarantees:

Updates/inserts are eventually consistent (i.e. given enough time all replicas
will sync). Conflict resolution is fully automatic (newest write wins with
microsecond granularity).

### Redundancy & Replication

The sharding across nodes and rebalances are fully automatic and transparent.
This means you will never have to worry or think about it.

Each table has a configurable replication factor N that controls on how many
servers each partition is stored. EventQL can tolerate up to N-1 failures and
still serve reads and writes for every partition. By default, N is 3 which
would make the number of tolerated failures 2.


Architecture Documentation
--------------------------

To dig deeper into the EventQL architecture, you could start with one of
these two blog posts. They're pretty high level but still might be good to get
started.

  - Columnar Storage and Parallel I/O
  - Dividing Infinity - Distributed Partitioning Schemes


Once you have gotten a general overview of the system, have a look at these
design documents and specifications for the individual subsystems:

  - Segment Based Replication
  - Partitioning
  - Binary Protocol
  - Storage Engine
  - Write Ahead Log
  - Parallel GroupBy
  - Distributed Join


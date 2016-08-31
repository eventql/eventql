Segment Based Replication
=========================

This document describes EventQL's segment based repliaction system.

Design Overview
---------------

EventQL stores data as rows in tables. Each table is split into a list of
partitions. Each partition contains a subset of the rows in the table. The
partition itself is stored on disk as a list of segment files. A segment file in
turn is an immutable data file containing one or more rows.

These segment files form an abstract log structured merge tree: when rows are
inserted or updated in a partition, the updates are first buffered in memory
and after a threshold is reached, written out to disk as a new segment file.

Now, the smallest unit of data that is replicated in EventQL are these segment
files.

Replication in EventQL is push-based, which means that a server that stores
a segment file is resposible for making sure that all other servers which should
store the segment file also have it.

On a very high level the replication algorithm is thus very simple: Every time
a server writes out a new segment file for a partition, it adds the segment file
into a queue to be pushed out to every other server that should have it. On
receiving a segment file, a server adds it to it's local log structured merge
tree.

If everything runs smoothly, all the EventQL replication does is to copy files
in the 1-500MB range between servers. This is pretty much as cheap and efficient
as it could get: we can easily saturate available IO and network bandwith with
just using a minimal amount of CPU ressources. Also, the algorithm transfers
only the minimal required amount (neglecting parity-based schemes) of data in
the steady operation case. I.e each row is copied exactly N-1 times between
servers where N is the replication factor.

Durability Considerations
-------------------------

The one thing we have to tradeoff for the performance gains that segment based
replication gives us is a bit of durability. Previously, with row-based
replication any insert would be written out to N different nodes as soon as
it was received by the cluster.

With segment based replication, each write is initially only durably stored
on one server in the cluster and only pushed out after a compaction timeout
of usually around 1 minute.

The implication of this is that with row based replication, as soon as a write
is confirmed we can be sure that it will never be rolled back unless three
machines simultaneously die with an unrecoverable hard drive. With segment
based replication however, when a single machine dies with an unrecoverable
hard drive error, up to 1 minute (or whatever the compaction timeout plus
replication delay is) of recent writes may be lost.

Given that most transactional databases (like MySQL or Postgres) do not give an
stronger guarantees than this and EventQL is specfically intended for analytical
usecases, we consider it a fair tradeoff.


Implementation Details
----------------------

    The procedure



Alternatives Considered
-----------------------

### Row-based replication?

The current row-based replication needs to run through the lsm insert code for
each insert at least 6 times (assuming a replication level of 3). This is a
considerable overhead -- the row only gets stored 3 out of those 6 times (and
rejected on the other 3 inserts) but just checking if the row should be accepted
or rejected is a fairly expensive operation and -- depending on cache contents --
might require multiple disk roundtrips.

### Operation-log-based replication?

The major issue is that we need to duplicate data (we need to store the actual
columnar data files plus an operation log). In the worst case this would increas
our disk footprint by more than 100% because the operation log is not columnar.

Of course, we could delete the operation log once we have pushed it out to all
servers. However, what do we do once a new server joins or a partition splits?
We would have to create a synthetic operation log from the cstable files.

Finally, any operation-log-based approach would incur the same overheads as the
row based replication.



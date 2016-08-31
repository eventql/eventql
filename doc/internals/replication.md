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

Each server keeps a list of local segments for each partition. The list contains,
for each segment file:

  - segment_id -- the segment file id
  - base_segment_id -- the id of the segment file on which this file is based
  - is_major -- if this is a major or a minor segment
  - acknowledged_servers -- the list of servers that have acknowledged the segment file

Additionally, a server keeps for each partition that it stores a "root segment
id". Every time a server creates a new segment file, it writes it out to disk,
adds a new entry to the segment list and sets the base_segment_id of the new
entry to the current root segment id and is_major to false. Then it sets the
current root segment id to the new segment id and enqueues the new segment for
replication.

### Compaction

When compacting a partition (i.e. folding a number of minor segments into the
next major segment) the server writes out a new segment file. The new
segment file has is_major set to true and base_segment_id to the id of the
most recent minor segment the compated segment was based on. It then sets the
root segment id to the id of the new major segment.

Note that the old minor and major segments do not get deleted immediately after
compaction to allow follwers to catch up. (The files are later deleted in the
replication procedure)

### Partition Leader

To ensure that replication will always be fast-forward in the steady case we
elect a "leader" for each partition using the coordination service. The exact
leader election algrithm should be pluggable and is not discussed here.

Suffice to say that it should always name exactly one of the replicas as the
leader. Still, even if for a short period of time two nodes consider themselves
the leader for a given partition, no corruption will occur. It will merely result
in a lot of unnecessary merges.

At any time, only the leader of a given partition accepts writes for that
partition. This implies that the leader is also the only server that produces
new segments for the partition. However any other replicat of the partition
can become the leader at any time.

### Replication Order

With respect to a given source and target server combination it is guaranteed
that the segments will be sent in sequential order. I.e. they will be sent in
the order in which they were added to the segments list and no segment will be
sent until it's predecessor segment has been acknowledged.

### Segment Merge and Fast-Forward



### Replication Procedure

If we are the leader for a given partition, execute this replication procedure:

    - For each replica R of the partition that is not ourselves:
      - For each segment file S in the partition
        - If the replica R is not included in the acknowledged_servers list for segment S
          - Offer the segment S to replica R
            - If the offer was declined with SEGMENT_EXISTS, add R to the acknowledged_servers list for S
            - If the offer was declined with OVERLOADED, abort and retry later
            - If the offer was declined with OUT_OF_ORDER, rmove the replica R from the acknowledged_server list and restart replication
            - If the offer was accepted, add R to the acknowledged_servers list for S
            - If an error occurs, abort and retry later


   - For each segment file S in the partition, from oldest to newest
      - Check if the segment S is (transitively) referenced by the root segment
        - If yes, check if the segment is referenced through a major segment
           - If yes, check if the segment S has been pushed to all followers
             - If yes, delete the segment
        - If no, merge the segment into the current partition and then delete it


If we are a follower for a given partition, execute this replication procedure:

    - For each segment file S in the partition
      - Check if partition leader is in the acknowledged_servers list for this segment
        - If yes, check if this segment is (trasitively) referenced by the root segment, discounting major segments
          - If no, delete the segment
        - If no, push the segment to the partition leader
          - If the offer was declined with SEGMENT_EXISTS, add R to the acknowledged_servers list for S
          - If the offer was declined with OVERLOADED, abort and retry later
          - If the offer was declined with OUT_OF_ORDER, remove the replica R from the acknowledged_server list and restart replication
          - If the offer was accepted, add R to the acknowledged_servers list for S
          - If an error occurs, abort and retry later


Upon receiving a segment from another node, execute this procedure:

    - Check if we are the leader for this partition
      - If yes,

      - If no


ABA Considerations
------------------

One ABA scenario occurs when a follower recives a major segment followed by
a minor segment, then another major segment and then another minor segment
that is based on the previous minor segment. From the senders point of view,
the previous minor segment has been acknowledged. From the receivers point of
view the minor segment is not based on the latest segment. This is handled by
responding with an out of order error code.



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


Code Locations
--------------




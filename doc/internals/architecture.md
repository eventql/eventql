8.1 Architecture
================

Each table's keyspace is split into a number of non-overlapping ranges called
"partitions" (like bigtable). 

Each partition is simultaneously served by N servers where N is called the
replication factor. Each server accepts queries and inserts for every partition
it serves (unlike bigtable). I.e. we can tolerate up to N-1 failures and still
serve reads and writes. In practice, N is often 3 which would make the number of
tolerated failures 2.

## Partition Location

Each table has a METADATA file that records the partition mapping. The TableConfig,
which is kept in the coordination service records these pieces of information:

    metadata_txnid: the currently valid METADATA transaction id for this table
    metadata_servers: the list of live metadata servers for this table

The METADATA file of each table is stored on N nodes which are recorded in the
TableConfig.

The METADATA file supports these general operations:


##### Change Metadata

To change the metadata, a change requester creates a change request. The change
request contains the modification (e.g. SPLIT_PARTITION), the transaction id
the change is based on and a new (random) transaction id.

To commit the change, the requests sends the change request to any coordinating
metadata server. The coordinationg metadata server generates a new METADATA file
for the new transcation id and applies the operation. The coordinating metadata
server then asks each other metadata server to store the new transaction.
Once a majority of the metadata servers (including the coordinating metadata
server) have confirmed that they have commited the new transaction to durable
storage, the coordinating metadata server performs a compare-and-swap update in
the coordination service to record the new transaction id as the current
transaction id. If the update suceeds, the change is commited and the coordinating
metadata server returns a success response to the requester.

The compare-and-swap update will fail if another transaction was (atomically)
commited in the meantime or the list of metadata servers changed. If the update
fails, the coordinating metadata server asks the other metadata servers to clean
up the aborted transaction file and returns an error the the requester.

##### Join Metedata Server

A new metadata server can join itself into the list of metadata servers for a given
table. Once the join was requested by e.g. the master, the metadata server reads
the latest metadata transaction id from the coordination service and tries to
download the METADATA file for that transaction from any of the currently live
metedata servers. Once it has successfully fetched the latest transaction it
performs a compare-and-swap update in the coordination service to add itself
to the list of live metadata servers. If another transaction was commited while
the new metadata server was trying to join (and hence, the locally stored
transaction isn't the most recent one), the update will fail and the procedure
is restarted from the beginning.


##### Remove Metedata Server

To leave the list of metadata servers, an update is written to the coordination
service to the remove the host from the list of metadata servers.


##### Background Metadata Replication

If a metadata server was unavailable for some time and does not have the
latest transaction stored it will not serve any metadata requests until it has
successfully retrieved the transaction from one of the other metadata servers.

##### Change Notification to affected Servers

Once a new metadata transaction was committed (by writing the new metadata
transaction to the TableConfig in the coordinator service) all other servers in
the cluster will reliably get notified of the change by the coordinator. When a
server sees a metadaaa change to a table, it will send a "partition discovery"
requerst for each partition that it has locally stored for the chnaged table to
one of the responsible metadata servers. This partition discovery request
contains the metadata transaction id and the name/id of the partition. The
response to the partition discovery request is called the partition discovery
response. The partition covery response contains the should-be status of the
partition on the requesting host ("LOAD", "SERVE" or "UNLOAD") and the list
of other servers (and partition ids, in case of a split) the data should be
pushed.

##### Metadata Transaction Sequence Numbers

The metadata transaction id is a random SHA1 hash. Additionally we store an
incrementing sequence number with each transaction id to allow us to order
transactions.


## Partition Assignment

Every partition is assigned to a list of N servers. Each server in this list
may handle read and write requests to this partition. Every table starts out
with a single partition that is then further subdivded (splitted) to create
more partitions.

Each partition stores two server lists:

    servers: the currently live servers that handle this partition
    joining_servers: the list of servers that are joining the server list

##### Initial Partition Assignment

To create a new table, the TableConfig is created atomically in the coordination
service. The initial TableConfig must point to an existing metadata file that
contains at least a single partition (covering the whole key range) and a list
of servers for this partition.

##### Partition Split

Any of the live servers for the partition may initiate a split. To split a
parent partition A into B1 and B1, the splitting server decides on the split
point and the initial server assignment for the new partitions B1 and B2 and then
commits a METADATA transaction recording an ongoing split into the entry for
partition A.

Once the split is commited, the splitting server immediately start to perform
the split operation. Other servers that host this partition will also start
to perform the split as soon as they are notified of the METADATA change.

The split procedure is simple, the current partition is read in and sent to
the new servers for the partition. Once at least the majority of servers for
both B1 and B2 have confirmed that they have committed the data to durable storage,
any splitting server commits a new METADATA transaction to finalize the split.
In this transcation, the partition A is removed from the partition list and
the new partitions B1 and B2 are added.

In case another server for the original partition A has not finished its split
yet it will get an update telling it that the partition A is no longer valid.
This case is naturally handled as part of the partition replication lifecyle.

##### Partition Replication and Lifecyle

Each server, locally, records an a map of server_id -> sequence pairs. In this
map, it stores until which sequence ID it has replicated a given partition to a
given server. Each partition is always notified of changes in the server list.

Partition replication and lifecyle is a simple algorithm:

      - For each partition P:
        - For each server which should store the data in partition P
           (considering splits and joining servers)
          - Check our local replication information for partition P and see if
            we have already pushed all records until the latest sequence to this
            server
          - If not push the unreplicated records to the server and, after it has
            confirmed them, record the new sequence
          - If this server has confirmed all records and we are a live server
            for this partition (i.e. not in the joining state), tell the server
            that inital replication has completed

        - If all servers have confirmed all records and we should _not_ store
          this partition on the local server, drop the partition

      - On every change in the server list, restart the replication for this
        affected partition

    - On every insert, restart the replication for the affected partition

To prevent ABA scenarios each partition to serer assignment has a unique id
that.

##### Server Join

To add a new server for a given partition, the server places itself in the joining
list for that partition. While the server is joining, it will not receive any read
or write requests for the partition but it will receive replicated records
from the other servers. Once any other (non joining) servers notifies the joining
server that initial replication has completed, the joining server commits another
METADATA transaction to mark itself as live.

Additionally there is a force-join option where a new server directly enters
the live server list. This should never be needed and used except in the case
where all servers for a given partition are down.


##### Server Leave

To remove a server from a given partition, a METADATA transaction is commited
to delete the server from either the server or the joining list of that partition.
Once the server is removed from the list, the normal replication and lifecyle
algorithm will eventually drop the local partition data once it is confirmed to
be replicated to all other live servers.

Of course, the server leave algorithm should ensure that no partition falls
below the number of replicas specified by the replication level. In the normal
case, the master handles all rebalances and initiates the leave operation only
when it is safe to remove a node.

## Timeseries Optimizations

##### Ahead-of-time split

the master detects the historical partition size and pre-splits partitions into
the future for active timeseries

##### Insert Smearing

One inherent scalability limit in our design as well as in the original bigtable
design is that - naively - every single point in time maps to exactly one 
partition, regardless of partitionsize.

So if all our writes are at the current wallclock time (i.e at any point in time,
all incoming writes are for the same part of the keyrange) then all writes are
handled by the same set of N machines. If we estimate that each machine can only
write up to 100 mb/s but we would like for each partition to be roughly 1GB in
size, we'll ultimately hit a botttlneck when more than 100 megabytes of new
at-the-current-time inserts per second are "lasered" at the same machine.

At this point, we can't make the problem go away by further splitting the hot
partition anymore while still satisfiny our constraint that a partiotion should
end up with roughly 1GB of that as  the problem is not the total volume of writes
(we can handle any volume of writes by making the partitions smaller), but the fact
that - at the chosen target partition size - filling up a any partition will take
longer than the wallclock timespan it covers.

To mitigate this bottleneck, we deploy a simple, unobtrusive optimization:

The lower insert limit is fixed by hardware constraints and can be reliably
inferred from the partition size (the bottleneck can only occur once partition
size becomes smaller than roughly $target_partition_size/100MB seconds).

We recognize that inserts are usually not sent directly to the responsible
partition server but first to any other server in our cluster. When a server
receives an insert for a partition whose keyrange is smaller than 1 minute, it
will randomly delay fowarding the insert for between 0 and 120 seconds. This
works out so that (in aggregate) any partition is evenly filled up over a
timewindow of at least 120 seconds (corresponding to at most ~10MB/s inbound
write load with a 1GB target partition size), regardless of total insert volume.

The downside of this approach is that for real-time insert volumes above 100mb/s
you'll see a good minute or so of delay for new writes. The upside is that the
optimization is simple, preserves all original semantics, requires no explicit
configuration and is completely transparent to the user while giving us horizontal
scalability well beyond gigabits of insert load.

Note that you can turn off this optimization both globally and per-insert.

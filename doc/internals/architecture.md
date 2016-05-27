

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


## Partition Assignment

Every partition is assigned to a list of N servers. Each server in this list
may handle read and write requests to this partition.

##### Initial Partition Assignment



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

## Partition Replication and Lifecyle

Each server, locally, records an a map of server_id -> sequence pairs. In this
map, it stores until which sequence ID it has replicated a given partition to a
given server. Each partition is always notified of changes in the server list.

Partition replication and lifecyle is a simple algorithm:

      - For each partition P:
        - For each server which should store the data in partition P
           (considering splits)
          - Check our local replication information for partition P and see if
            we have already pushed all records until the latest sequence to this
            server
          - If not push the unreplicated records to the server and, after it has
            confirmed them, record the new sequence

        - If all servers have confirmed all records and we should _not_ store
          this partition on the local server, drop the partition

      - On every change in the server list, restart the replication for this
        affected partition

    - On every insert, restart the replication for the affected partition

To prevent ABA scenarios each partition to serer assignment has a unique id
that.




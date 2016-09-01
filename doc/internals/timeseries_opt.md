
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



3.2 Partitioning
================

Tables in EventQL are internally split into many partitions that can be distributed
among many machines and queried in parallel, allowing you to horizontally scale
tables far beyond what a single machine could handle. This concept is sometimes referred
to as "massively parallel database architecture".

The partioning is more or less transparent to users. If you want to learn about the
intricate details of the algorithm, have a look at the internals section.
Still there are a few things that you have to consider while designing your data
model for EventQL:

## The Primary Key

The primary key consists of one or more columns from the table schema. The primary
key is automatically unique. There can't be two rows with the exactly same
primary key value (i.e. the same value in all columns that are included in the 
primary key) in the same table.

Two consecutive writes with the same primary key value are treated as an insert
followed by an update - that is, every insert with a primary key value equal to
that of another row that already exists will replace that original row.

#### Partitioning Key

The partitioning key is defined as exactly one column from the table schema that must
also be included in the primary key. Another way to see it is that the partinioning
key is a subset of the primary key.

The partitioning key is used to distribute rows across machines. The brief description
of how this works is that rows with the same partioning key will end up on the
same machine and rows with similar partioning keys will also be grouped together.

This scheme allows EventQL to make an extremely quick decision which servers
need to be asked for data in a full table scan.

When storing timeseries or event streams the partitioning key will almost always
be `time` as this allows to efficiently restrict scans on a specific time window.

#### Example

Here is a simple example schema for a table that stores temperature measurements
and its primary and partitioning key:

    CREATE TABLE ev.temperature_measurements (
      collected_at    DATETIME,
      sensor_id       STRING,
      temperature     DOUBLE,
      PRIMARY KEY(time, sensor_id)
    );

We choose `time` as the partition key as this allows us to efficiently execute
queries that are restricted on a specific time window. I.e. "aggregate over all
events between 2016-01-15 and 2016-01-30".

Now, if we would have used time as both partitioning key and primary key we
could only have store one measurement at each time. What if we have multiple
sensors reporting data? Two or more of those sensors could send a measurement
at the same time, so we also include the sensor_id in the primary key. Now we
can have one measurement per sensor_id and time combination.


## Finite Partitions & Timeseries Data

The partitioning of a table is fully dynamic. EventQL will dynamically
re-partition the table as you add more data to keep each partition in the 500MB
to 1GB range. So there is no need to specify a maximum/total number of shards at
creation time.

This is good because you don't need to know how many rows with which
primary key distribution you'll eventually insert from the get-go. With
dynamic partitioning, you just create a table and start inserting data and the
partitioning will adapt to the distribution of the data over time.

The way EventQL achieves this is in principle by starting each table with
a single partition and then subdiving ("splitting") the partition(s) into equal
parts as they get too large.

However, you can optionally give EventQL an "educated guess" about the size
of each partition that you expect. The configuration option is called
`finite partition size` and setting it allows an optimization to kick in that
reduces the number of splits performed and therefore the total network bandwith.

The finite partition size is a time window/duration in microseconds. You should
ideally choose this value so that roughly 500MB-1GB of new data will arrive in
the time window.

So, for example, if you expect around 10GB of data a day, 2 hours would be a
good value. If you expect 1000GB of new data a day, 1 minute is a good value.
If you expect 10TB of new data a day, set the finite partition size to 10
seconds.

Note that EventQL will still dynamically split and re-partition the table if it
becomes necessary -- the finite partition size is merely a hint. You can update
the finite partition size hint at any time and it won't cause trouble if your
estimation is off.

For high-volume timeseries data, it is _highly recommended_ to set the finite
partition size.

To set the finite partition size when creating a table using SQL you can use
this syntax. Please refer to the "Creating Tables" and "HTTP API Reference"
pages for detailed information.

    CREATE TABLE high_volume_logging_Data (
      collected_at    DATETIME,
      event_id        STRING,
      value           DOUBLE,
      PRIMARY KEY(time, event_id)
    ) WITH finite_partition_size = 600000000; -- 10 minutes ~ 100GB/day


## Secondary indexes

EventQL does not currently support secondary indexes. Lookup by partition/primary
key is the only optimized operation. That means every query that is not restricted
on the primary key requires a full table scan. Note that a full table scan is
parallelized and distributed among many machines so it's still very fast.

Also, not supporting secondary indexes does _not_ imply that you can't filter by
anything other than the primary key like in some key value stores. You can. All
WHERE, GROUP BY and JOIN BY clauses that are valid in standard SQL are supported.

If you need to have fast access to _individual_ records by _more than one_ dimension
you have to denormalize and insert the record multiple times in different tables
that are indexed in those respective dimensions.


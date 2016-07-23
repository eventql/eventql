3.2 Partitioning
================

Tables in EventQL are internally split into many partitions that can be distributed
over many machines and queried in parallel, allowing you to horizontally scale
tables far beyond what a single machine could handle. This concept is sometimes referred
to as "massively parallel database architecture".

The partioning is more or less transparent to users. If you want to learn about the
intricate details of the algorithm, have a look at the internals section.
Still there are few things that you have to consider while designing your data
model for EventQL:

## The Primary Key

The primary key consists of one or more columns from the table schema. The primary
key is automatically unique. There can't be two rows with the exactly same
primary key value (i.e. the same value in all columns that are included in the 
primary key) in the same table.

Two consecutive writes with the same primary key value are treated as an insert
followed by an update 0 that is, every insert with a primary key value equal to
that of another record that already exists will replace that original record.

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


## No secondary indexes

EventQL does not support secondary indexes. Lookup by partition/primary key is
the only optimized operation. That means every query that is not restricted on
the primary key requires a full table scan. Note that a full table scan is
parallelized and distributed across many machines so it's still very fast.

Also, not supporting secondary indexes does _not_ imply that you can't filter by
anything other than the primary key like in some key value stores. You can. All
WHERE, GROUP BY and JOIN BY clauses that are valid in standard SQL are supported.

If youneed to have fast access to _individual_ records by _more than one_ dimension
you have to denormalize and insert the record multiple times in different tables
that are indexed in those respective dimensions.

However, most users probably won't need this. Access by a single primary key is
always fast and if all you care about are aggregate queries for analytics use cases
and retrieving and updating invdividual events by id, you're good to go.

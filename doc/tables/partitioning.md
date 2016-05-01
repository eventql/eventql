2.1.2 Partitioning
==================

Tables in EventQL are internally split into many partitions that can be distributed
over many machines and queried in parallel, allowing horizontally scale tables far
beyond what a single machine could handle. This concept is sometimes referred
to as "massively parallel database architecture".

The partioning is more or less transparent to you. If you want to learn about the
intricate details of the algorithm, have a look at the internals section.
Still there are few things that you have to consider when designing your data
model for EventQL:

## Mandatory Primary Key

#### Partitioning Key
#### Primary Key

## No secondary indexes

EventQL does not support secondary indexes. Lookup by partition/primary key is
the only optimized operation. That means every query that is not restricted on
the primary key requires a full table scan. Note that a full table scan is
parallelized and distributed across many machines so it's still very fast.

Also, not supporting secondary indexes does _not_ imply that you can't filter by
anything other than the primary key like in some key value stores. You can. All
WHERE, GROUP BY and JOIN BY clauses that are valid in standard SQL are supported.

So why don't we support secondary indexes? There simply is no
database design in existence that can scale out horizontally and reasonably
restrict table scans on multiple dimensions at the same time.

If you really need to have fast access to _individual_ records by _more than
one_ dimension your best bet is to denormalize and insert the record multiple times
in different tables that are indexed in those respective dimensions. 

However, most users won't need this. Access by a single primary key is always
fast and if all you care about are aggregate queries for analytics use cases
and retrieving and updating invdividual events by id, you're good to go.

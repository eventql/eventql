# EventQL

[![Build Status](https://travis-ci.org/eventql/eventql.png?branch=master)](http://travis-ci.org/eventql/eventql)

EventQL is a distributed, columnar database built for large-scale data collection
and analytics workloads. It can handle a large volume of streaming writes and
runs super-fast SQL and MapReduce queries.

**More information:**
[Documentation](http://eventql.io/),
[Download](https://eventql.io/download/),
[Architecture](https://eventql.io/documentation/internals/architecture/),
[Getting Started](https://eventql.io/documentation/getting-started/first-steps/)


## Features

This is a quick run-through of EventQL's key features to get you excited. For
more detailed information on these topics and their caveats you are kindly
referred to the documentation.

- **Automatic partitioning.** Tables are transparently split into partitions using
a primary key and distributed among many machines. You don't have to configure
the number of shards upfront. Just insert your data and EventQL handles the rest.

- **Idempotent writes.** Supports primary-key based INSERT, UPSERT and DELETE
operations. You can use the UPSERT operation for easy exactly-once ingestion
from streaming sources.

- **Compact, columnar storage.** The columnar storage engine allows EventQL to
drastically reduce its I/O footprint and execute analytical queries orders of
magnitude faster than row-oriented systems.

- **Standard SQL support.** (Almost) complete SQL 2009 support. (It does JOINs!)
Queries are also automatically parallelized and executed on many machines in
parallel

- **Scales to petabytes.** EventQL distributes all table partitions and queries
among a number of equally privileged servers. Given enough machines you can store
and query thousands if terrabytes of data in a single table.

- **Streaming, low-latency operations.** You don't have to batch-load data
into EventQL - it can handle large volumes of streaming insert and update
operations. All mutations are immediately visible and minimal SQL query latency
is ~0.1ms.

- **Timeseries and relational data.** The automatic partitioning supports
timeseries as well as relational and key value data, as long as there is a good
primary key. The storage engine also supports REPEATED and RECORD types so
arbitrary JSON objects can be inserted into rows.

- **HTTP API.** The HTTP API allows you to use query results in any application
and easily send data from any application or device. EventQL also supports a
native TCP-based protocol.

- **Fast range scans.** Table partitions in EventQL are ordered and have a
defined keyrange, so you can perform efficient range scans on parts of the
keyspace.

- **Hardware efficient**. EventQL is implemented in modern C++ and tries to
achieve maximal performance on commodity hardware by using vectorized execution
and SSE instructions.

- **Highly Available**. The shared-nothing architecture of EventQL is highly
fault tolerant. A cluster consists of many, equally privileged nodes
and has no single point of failure.

- **Self-contained.** You can set up a new cluster in minutes. The EventQL server
ships as a single binary and has no external dependencies except Zookeeper or a
similar coordination service.


## Use Cases

Here are a few example scenarios that are particularly well suited to EventQL's
design:

- Storage and analysis of streaming event, timeseries or relational data
- High volume event and sensor data logging
- Joining and correlating of timeseries data with relational tables

#### Non-goals

Note that EventQL is built around specific design choices that make it an
excellent fit for real-time data analytics processing (OLAP) tasks, but also
mean it's not well suited for most transactional (OLTP) workloads.


## Build

Before we can start we need to install some build dependencies. Currently
you need a modern c++ compiler, libz, autotools and python (for spidermonkey/mozbuild)

    # Ubuntu
    $ apt-get install clang make automake autoconf libtool zlib1g-dev

    # OSX
    $ brew install automake autoconf

To build EventQL from a distribution tarball:

    $ ./configure
    $ make
    $ sudo make install

To build EventQL from a git checkout:

    $ git clone git@github.com:eventql/eventql.git
    $ cd eventql
    $ ./autogen.sh
    $ ./configure
    $ make V=1
    $ src/evql -h

To run the test suite:

    $ make check


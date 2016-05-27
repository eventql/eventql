# EventQL [![Build Status](https://secure.travis-ci.org/eventql/eventql.png)](http://travis-ci.org/eventql/eventql)

EventQL is a distributed, column-oriented database built for large-scale event
collection and analytics. It runs super-fast SQL and JavaScript queries.

Documentation: [eventql.io](http://eventql.io/)

## Build

Before we can start we need to install some build dependencies. Currently
you need a modern c++ compiler, libz, autotools and python (for spidermonkey/mozbuild)

    # Ubuntu
    $ apt-get install clang++ cmake make automake autoconf zlib1g-dev

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

    $ make test

## Features

* Supports INSERT, UPDATE and DELETE operations (Log-Structured-Merge-Tree design)
* Supports flat and complex/nested tables (OBJECT/ARRAY column types). Table rows can be inserted and retrieved as JSON objects.
* Columnar Storage Engin -- Tightly packed storage for all primitive data types (see Column Storage Types). Queries only read required data from referenced columns
* (Almost) complete SQL 2009 support. (Yes, it does JOINs)
* Run (streaming) MapReduces in JavaScript, mix JavaScript and SQL
* Support for user defined functions in JavaScript
* Fully transparent query caching when the same query is repeatedly executed on (partially) unchanged data
* Distributed Storage and Query Execution -- Tables are split into ordered partitions by primary key
* Queries and jobs are automatically parallelized and executed on many machines in parallel
* Strong eventual consistency for inserts, updates and deletes
* Scales to hundreds of terrabytes per table, thousands of tables per cluster
* High Availability
* Shared nothing" design. A Z1 cluster consists of many independent tableservers and one master. The master is not directly involved in replication, queries and inserts/updates.
* Written in modern C++11 with extensive documentation. Commercial support available

# EventQL

EventQL is a distributed, column-oriented database built for large-scale event
collection and analytics. It runs super-fast SQL and JavaScript queries.

Documentation & Examples: [eventql.io](http://eventql.io/)

[![Build Status](https://travis-ci.org/eventql/eventql.png?branch=master)](http://travis-ci.org/eventql/eventql)

## Features

* Tables are transparently split into ordered partitions by primary key and distributed onto many machines
* Supports INSERT, UPDATE and DELETE operations
* Supports flat and complex/nested tables (OBJECT/ARRAY column types). Table rows can be inserted and retrieved as JSON objects.
* Rows are stored in a column oriented fashion. Queries only read required data for referenced columns.
* (Almost) complete SQL 2009 support. (It does JOINs!)
* Queries are also automatically parallelized and executed on many machines in parallel
* Scales to hundreds of terrabytes per table, thousands of tables per cluster
* "Shared nothing" design. An EventQL cluster consists of many equally privileged servers
* Fully transparent query caching when the same query is repeatedly executed on (partially) unchanged data
* Written in modern C++11 with extensive documentation. Commercial support available

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

To run EventQL Server with Docker:

    $ git clone git@github.com:eventql/eventql.git
    $ cd eventql/contrib/docker/server
    $ chmod +x run.sh && ./run.sh

The server will start on port 9175

To run the test suite:

    $ make test

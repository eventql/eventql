libcstable v0.2.0
=================

[![Build Status](https://travis-ci.org/zscale/libcstable.svg?branch=master)](https://travis-ci.org/zscale/libcstable)

libcstable allows you to write and read cstables (columnar storage tables).

A cstable is a disk-based data structure that stores a collection of "records".
The cstable has a strict schema which each record must adhere to. Still, nesting
and repetition within records is fully supported - this means you can store,
for example, a list of arbitrarily complex JSON objects or protobufs messages
  in a cstable.

The layout of the data on disk is "column oriented", i.e. rather than storing
a list of records, one record after another, we store a list of "columns", one
column after another. A column contains the values of a particular field from
_all records_.

The key features of libcstable:

  - Large list of tightly packed column storage formats (see below)
  - Record shredding and re-materialization to/from Protobuf and JSON
  - Streaming writes -- write huge tables with very little memory
  - Snapshot consistency when appending to tables that are being read concurrently
  - Well-documented, modern C++11 code
  - Battle tested in production

Columnar Storage
----------------

This concept is best illustrated by example


Column Types
-----------

### Logical Types

  - BooleanColumn
    - UINT32_BITPACKED (8 bools per byte)
    - LEB128_RLE

  - UnsignedIntegerColumn
    - UINT32_PLAIN
    - UINT64_PLAIN
    - UINT32_BITPACKED
    - LEB128
    - LEB128_RLE
    - LEB128_DELTA

  - SignedIntegerColumn

  - DoubleColumn
    - IEE754_PLAIN

  - StringColumn
    - STRING_PLAIN
    - STRING_CATALOG

  - DateTimeColumn
    - UINT64_PLAIN
    - LEB128
    - LEB128_RLE
    - LEB128_DELTA

### Storage Types

  - UINT32_PLAIN
  - UINT32_BITPACKED
  - UINT64_PLAIN
  - LEB128
  - LEB128_RLE
  - LEB128_DELTA
  - IEE754_PLAIN
  - STRING_PLAIN
  - STRING_CATALOG


Shredding & Materializing Records
---------------------------------


Sources / References
--------------------

[1] S. Melnik, A. Gubarev, J. Long, G. Romer, S. Shivakumar, M. Tolton, T. Vassilakis (2010). Dremel: Interactive Analysis of Web-Scale Datasets (Google, Inc.)


License
-------

Copyright (c) 2014 Paul Asmuth

libcstable is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License v3.0. You should have received a
copy of the GNU General Public License along with this program. If not, see
<http://www.gnu.org/licenses/>.

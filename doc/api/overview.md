7. API Reference
================

EventQL supports a number of APIs that allow you to programmatically insert
rows, create and alter tables and execute queries.

### [HTTP API](http/)

The EventQL HTTP API gives you full access to EventQL via HTTP. All EventQL
operations can be executed via this API. The EventQL CLI uses the HTTP API to
access EventQL.


### [MapReduce &mdash; JavaScript API](javascript_mapreduce)

The MapReduce JavaScript API allows you to write JavaScript programs that scan,
aggregate and transform data in EventQL. JavaScript programs that use the MapReduce
API are automatically distributed and parallelized within EventQL.

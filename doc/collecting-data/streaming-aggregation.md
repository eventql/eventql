Streaming Aggregation
=====================

EventQL can naturally handle hundreds of thousands of regular writes per second
(if you run it on enough servers and the data is partitioned correctly, see the
High Volume Timeseries & Logs Page).

Still, for some use cases it makes sense to pre-aggregate the data in a streaming
fashion before permanently storing a smaller pre-aggregated stream of rows in
EventQL. An example could be ....


### Custom Streaming Aggregation

There are numerous systems that are built epspecially to support real-time
streaming aggregation of data. Some examples are Apache's Kafka or Flink. For
complex streaming aggregation jobs, we recommend that you use one of these
stream processing systems to run your custom streaming aggregation and then
submit the pre-aggregated results for permanent storage to EventQL.

Check out these examples on how to run a stream processing service together
with EventQL:



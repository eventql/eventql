# ZooKeeper
We store three pieces of data under the following paths:

    ClusterConfig:     /eventql/<cluster_name>/config
    NamespaceConfig:   /eventql/<cluster_name>/namespaces/<namespace>/config
    TableConfig:       /eventql/<cluster_name>/namespaces/<namespace>/tables/<table_name>
    PartitionConfig:   /eventql/<cluster_name>/namespaces/<namespace>/tables/<table_name>/<partition>


### things zookeeper does:

- stores a tree of config data. each node can have data + children
- clients get notified on changes in the tree (optional: only for some subtrees
- must guarantee strong durability and sequenital consistency (i.e. no write is _ever_ lost and there can be no conflicts [each write grabs a lock per key])
- must be possible to make HA somehow
- no key value is ever larger than ~1MB

Example Tree:

     /eventql/<cluster>/namespaces/<namespace>/tables/<table>/<partition>

each partition has one such key and represents ~2-5GB of usable data. so in a 
very large cluster we expect up to 10 million (20-50PB) partitions. the values
stored in each partition key are only a few bytes in practice.


### things zookeeper does not do (limits):

zookeeper requires us to set a watcher on each individual node if we want to
watch the whole tree. since each server in the cluster needs to watch the three
this works out to

  num_watches = num_nodes X num_servers 

so for example

  100 servers x 10_000 nodes (20-50TB data stored) = 100k watchers [ should still work ]
  200 servers x 100_000 nodes (200-500TB data stored) = 20 million watchers 

this latter case is apparently already beyond ZK limits, even though we're only
talking about roughly ~6MB of configuration data total!

https://issues.apache.org/jira/browse/ZOOKEEPER-1177

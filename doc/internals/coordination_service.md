8.3 Coordination Service
========================

We store three pieces of data under the following paths:

    ClusterConfig:     /eventql/<cluster_name>/config
    NamespaceConfig:   /eventql/<cluster_name>/namespaces/<namespace>/config
    TableConfig:       /eventql/<cluster_name>/namespaces/<namespace>/tables/<table_name>


### things zookeeper does and we need:

- stores a tree of config data. each node can have data + children
- clients get notified on changes in the tree (optional: only for some subtrees
- must guarantee strong durability and sequenital consistency (i.e. no write is _ever_ lost and there can be no conflicts [each write grabs a lock per key])
- allows compare-and-swap updates on a key
- must be possible to make HA somehow
- no key value is ever larger than ~1MB


### things zookeeper does that we don't need:

- guarantees about timeliness (data is never stale, etc) -- evqld actually expects the config data to be eventually consistent (it expects to always have an old state locally)


### things zookeeper does not do that we will need eventually (limits):

zookeeper requires us to set a watcher on each individual node if we want to
watch the whole tree. since each server in the cluster needs to watch the tree
this works out to

    num_watches = num_nodes X num_servers 

so for example

  - 100 servers x 10_000 tables = 100k watchers [ should still work ]
  - 200 servers x 100_000 tables = 20 million watchers [ does probably not work anymore ]

this latter case is apparently already beyond ZK limits, even though we're only
talking about roughly ~6MB of configuration data total!

https://issues.apache.org/jira/browse/ZOOKEEPER-1177

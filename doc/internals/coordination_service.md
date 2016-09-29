8.2 Coordination Service
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


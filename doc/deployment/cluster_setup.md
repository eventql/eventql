2.5.1 Setting up a new cluster
==============================

This page will guide you through setting up an EventQL cluster. With the default
settings, a cluster should consist of at least four nodes.

For our example, we will run the four EventQL server instances on a single machine
as it makes the guide somewhat simpler to follow. In production each instance
should run on a different host.

#### Step 1: Setting up the coordinator service

EventQL requires a coordinaton service like ZooKeeper to store small amounts
of cluster metadata. In the future, we are planning to implement other backends
like etcd, but currently Zookeeper is the only supported coordination service.

This guide assumes you have a zookeeper instance running on `localhost:2181`. You
should change the parameters accordingly for your setup.

If you don't have a running zookeeper instance, please refer to the
[official zookeeper tutorial](...).

#### Step 2: Creating the configuration files

#### Step 3: Creating the cluster


#### Step 4: Adding the intial servers


#### Step 5: Starting the EventQL servers

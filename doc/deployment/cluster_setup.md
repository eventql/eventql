2.5.1 Setting up a new cluster
==============================

This page will guide you through setting up an EventQL cluster. With the default
settings, a cluster should consist of at least four nodes.

For our example, we will run the four EventQL server instances on a single machine
as it makes the guide somewhat simpler to follow. In production each instance
should run on a different host.

### Step 1: Setting up the coordinator service

EventQL requires a coordinaton service like ZooKeeper to store small amounts
of cluster metadata. In the future, we are planning to implement other backends
like etcd, but currently Zookeeper is the only supported coordination service.

This guide assumes you have a zookeeper instance running on `localhost:2181`. You
should change the parameters accordingly for your setup.

If you don't have a running zookeeper instance, please refer to the
[official zookeeper tutorial](https://zookeeper.apache.org/doc/r3.3.3/zookeeperStarted.html).

### Step 2: Creating the configuration files

The eventql binaries allow you to load configuration variables from a configuration
file or specify them on the command line. For this guide, we will create one
configuration file that stores the common configuration variables and put the
instance-specific options onto the command line.

Create a file `/etc/evqld.conf` and put in these contents:

    [cluster]
    name=mycluster
    coordinator=zookeeper
    zookeeper_hosts=localhost:2181
    allowed_hosts=0.0.0.0/0

    [server]
    client_auth_backend=trust

The `cluster.name` option contains the cluster name (you can run multiple EventQL
clusters with a single coordinator). You may set the cluster name to whatever you
want.

The `cluster.coordinator` option specifies that we are going to use zookeeper
as our coordination service.

The `cluster.zookeeper_hosts` contains a comma separated list of zookeeper hosts to connect
to.

For a full list of supported configuration options please refer to the [Configuration](../../configuration)
page.


### Step 3: Creating the cluster

The next step is to create our cluster using the evqlctl program. Run this command
to create the cluster:

    $ evqlctl -c /etc/evqld.conf cluster-create

Alternatively, we could also have specified all configuration options on the
commandline like this:

    $ evqlctl cluster-create \
        -C cluster.name=mycluster \
        -C cluster.coordinator=zookeeper \
        -C cluster.zookeeper_hosts=nue01.prod.fnrd.net:2181


### Step 4: Adding the initial servers

Now our cluster is ready and we can start adding servers. Each server needs
a unique name. We will call ours "node1" through "node4" but you can change
this to use your own naming scheme.

    $ evqlctl -c /etc/evqld.conf cluster-add-server --server_name "node1"
    $ evqlctl -c /etc/evqld.conf cluster-add-server --server_name "node2"
    $ evqlctl -c /etc/evqld.conf cluster-add-server --server_name "node3"
    $ evqlctl -c /etc/evqld.conf cluster-add-server --server_name "node4"

### Step 5: Starting the EventQL servers

We're almost done. All that is left to do is to start the four servers.

If trying this out on your local machine, open four terminal windows. If you are
setting up on multiple machines, run each command on the respective machine.

Create the data directories first:

    $ mkdir /var/evql/node{1,2,3,4}

Then start the four servers:

    $ evqld -c /etc/evqld.conf -C server.name=node1 --listen localhost:9175 --datadir /var/evql/node1/
    $ evqld -c /etc/evqld.conf -C server.name=node2 --listen localhost:9176 --datadir /var/evql/node2/
    $ evqld -c /etc/evqld.conf -C server.name=node3 --listen localhost:9177 --datadir /var/evql/node3/
    $ evqld -c /etc/evqld.conf -C server.name=node4 --listen localhost:9178 --datadir /var/evql/node4/

Note that when a server joins the cluster, it will publish its `listen` address
to the other servers. So it is important that all other servers can open
connections to whatever address is specified.

For our example (we assume you run this on your development machine) we use
localhost with four different ports as the listen address. However, when running
on multiple machines, you can't use `localhost` or `0.0.0.0` - you have to specify
the externally reachable ip address of each server.

That's it! Our cluster is running, let's create a database:

    $ evqlctl -c /etc/evqld.conf database-create --database "mydb"

You should now be able to connect to the cluster and start executing
queries:

    $ evql -h localhost -p 9175 -d mydb

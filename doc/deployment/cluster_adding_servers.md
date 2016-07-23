2.5.2 Adding Servers
====================

Adding new servers to an EventQL cluster is an instant, cheap operation. In
most cases, adding a new server will not result in any rebalances of data (i.e.
it doesn't create any additional network traffic or load on the servers).

In general, having more servers in an EventQL cluster means you'll be able to
more or less linearily store more data and execute queries faster. So adding
more servers is always good.

To add a new server, you must first choose a name for the server. This can be
whatever you want, e.g. the machine's hostname. We will use `nodeX` for our
example - change accordingly.

The general command to add a new server is:

    $ evqlctl cluster-add-server --server_name "nodeX"

However, you might have to pass configuration options. Assuming you are running
zookeeper as the coordination service on `localhost:2181` and the cluster name
is `mycluster`, the full command line could look like this:


    $ evqlctl cluster-add-server \
        -C cluster.name=mycluster \
        -C cluster.coordinator=zookeeper \
        -C cluster.zookeeper_hosts=nue01.prod.fnrd.net:2181 \
        --server_name "nodeX"



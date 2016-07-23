2.5.3 Removing Servers
======================

Sometimes you need to remove a server from a cluster. Either because the hardware
or the operation system has become defect or simply because you are re-provisioning
the datacenter.

Thera are two methods of removing a cluster, called `soft leave` and `hard leave`.
The difference between these two methods lies in a tradeoff between how quickly
the server can be removed and not violating any redundancy guarantees.

#### Soft Leave

When `soft-leaving` a server, a flag is added to that servers configuration that
tells the cluster not to put any new data onto the server and to start rebalancing
the data owned by the leaving server to other nodes.

However, this rebalancing algorithm is designed not to create excessive network
or IO/CPU load by rebalancing all the data at once and to never violate the
redundancy guarantees.

This means that the data will be moved from the leaving server at a fairly slow
pace and you should not actually disconnect the leaving server until all data was
moved away from there.

Depending on the amount of data stored on the leaving server, cluster size and
available network bandwith a slow leave operation might take many hours to days
to complete.

The slow leave operation is therefore applicable, if you are reprovisioning and
should not be used to remove defect hosts.

The general command to perform a soft leave is (assuming you want to remove `nodeX`)

    $ evqlctl cluster-remove-server --server_name "nodeX" --soft

However, you might have to pass configuration options. Assuming you are running
zookeeper as the coordination service on `localhost:2181` and the cluster name
is `mycluster`, the full command line could look like this:

    $ evqlctl cluster-remove-server \
        -C cluster.name=mycluster \
        -C cluster.coordinator=zookeeper \
        -C cluster.zookeeper_hosts=nue01.prod.fnrd.net:2181 \
        --server_name "nodeX" \
        --soft


#### Hard Leave

The `hard leave` operation will immediately remove the server from the cluster
and mark all data previously stored on the node as "dead". After this happens,
the other (live) servers will detect that some partitions have fewer replicas
than they should have and will immediately start to assign new servers to those
partitions and start replicating. 

This means a hard leave is instant: you can immediately disconnect the server
after the hard-leave operation has completed. A hard leave is also applicable
for defect hardware (where the machine/server is already dead and is removed from
  the cluster after-the-fact).

However, a hard leave results in an instant rebalance that might create some
network load and will temporarily reduce the number of replicas
(i.e. the redundancy level) for some data. So you have to be careful not to
execute too many hard leaves at once as this might lead to data loss.

Do not use the hard leave to reprovision the cluster, but only for defect nodes.

The command to perform a hard leave is (assuming you want to remove `nodeX`)

    $ evqlctl cluster-remove-server --server_name "nodeX" --hard


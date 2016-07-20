4.3 evqlctl Reference
==================

evqlctl is the EventQL command line tool to manage a cluster.

    Usage: $ evqlctl [OPTIONS] <command> [<args>]

       -c, --config <file>       Load config from file
       -C name=value             Define a config value on the command line
       -?, --help <topic>        Display a command's help text and exit
       -v, --version             Display the version of this binary and exit



evqlctl-cluster-add-server - Add a server to an existing cluster.

      Usage: evqlctl cluster-add-server [OPTIONS]
         --cluster_name <node name>       The name of the cluster to add the server to.
         --server_name <server name>      The name of the server to add.


evqlctl-cluster-create - Create a new cluster.

      Usage: evqlctl [OPTIONS]
        --cluster_name <node name>       The name of the cluster to create.


evqlctl-cluster-remove-server - Remove an existing server from an existing cluster.

      Usage: evqlctl [OPTIONS]
        --cluster_name <node name>       The name of the cluster to add the server to.
        --server_name <server name>      The name of the server to add.
        --soft                           The name of the server to add.
        --hard                           The name of the server to add.



evqlctl-cluster-status - Display the current cluster status.

      Usage: evqlctl [OPTIONS] -cluster-status
        --master <addr>       The url of the master.


evqlctl-namespace-create - Create a new namespace.

      Usage: evqlctl [OPTIONS] -namespace-create
      --cluster_name <node name>       The name of the cluster
      --namespace <namespace name>     The name of the namespace to create


evqlctl-rebalance - Rebalance.

      Usage: evqlctl [OPTIONS] 
        --cluster_name <node name>       The name of the cluster.


evqlctl-table-split - Split partition

      Usage: evqlctl table-split [OPTIONS]
        --namespace              The name of the namespace.
        --cluster_name           The name of the cluster.
        --table_name             The name of the table to split.
        --partition_id           The id of the partition to split.
        --split_point 

evqlctl-table-set-primary-key - Set primary key

      Usage: evqlctl table-set-primary-key [OPTIONS]
        --namespace              The name of the namespace.
        --cluster_name           The name of the cluster.
        --table_name             The name of the table to split.
        --primary_key            The primary key (comma separated)


evqlctl-table-config-set - Set table config parameters

      Usage: evqlctl table-config-set [OPTIONS]
        --database               The name of the database to modify.
        --table_name             The name of the table to modify.
        --param                  The parameter to set
        --value                  The value to set the parameter to


evqlctl-table-disable-replication - Disable replication

      Usage: evqlctl table-disable-replication [OPTIONS]
        --namespace              The name of the namespace.
        --cluster_name           The name of the cluster.
        --table_name             The name of the table to split.

evqlctl-cluster-set-allocation-policy - Set allocation policy for a server.

      Usage: evqlctl [OPTIONS]
        --cluster_name <node name>       The name of the cluster to add the server to.
        --server_name <server name>      The name of the server to add.

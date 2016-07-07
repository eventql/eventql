4.3 evqlctl Reference
==================

evqlctl is the EventQL command line tool to manage a cluster.

    Usage: $ evqlctl [OPTIONS] <command> [<args>]

       -c, --config <file>       Load config from file
       -C name=value             Define a config value on the command line
       -?, --help <topic>        Display a command's help text and exit
       -v, --version             Display the version of this binary and exit

    evqctl commands:
       cluster-add-server        Add a server to an existing cluster.                                            
       cluster-create            Create a new cluster.                                                           
       cluster-remove-server     Remove an existing server from an existing cluster.                             
       cluster-status            Display the current cluster status.                                             
       namespace-create          Create a new namespace.                                                         
       rebalance                 Rebalance.                                                                      
       table-split               Split partition                                                                 

    See 'evqlctl help <command>' to read about a specific subcommand.

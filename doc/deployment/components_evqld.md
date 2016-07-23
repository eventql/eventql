2.3.1 evqld
===========

evqld, also known as the EventQL server, is the main server program. When the
EventQL server starts, it listens for network connections and HTTP requests from
client programs and executes inserts and queries on behalf of those clients.

The EventQL server can be run in two modes called "standalone mode" and "cluster
mode".

In the standalone mode evqld runs as a standalone process on a single machine and
has no further dependencies. The standalone mode is ideal for evaluating and development
setups as it allows you to have a server up and running within seconds. To learn
more about how to run EventQL in standalone mode, read the [First Steps](...) or
[Standalone](...) pages.

In the cluster mode, many instances of the evqld binary are run on a number of
machines. When running in cluster mode, the individual EventQL Server instances
connect to a coordination service like Zookeeper.

The evqld program has somey options that can be specified at startup. For a
complete list of options, run this command:

    $ evqld  --help

    Usage: $ evqld [OPTIONS]

       -c, --config <file>       Load config from file
       -C name=value             Define a config value on the command line
       --standalone              Run in standalone mode
       --datadir <path>          Path to data directory
       --listen <host:port>      Listen on this address (default: localhost:9175)
       --daemonize               Daemonize the server
       --pidfile <file>          Write a PID file
       --loglevel <level>        Minimum log level (default: INFO)
       --[no]log_to_syslog       Do[n't] log to syslog
       --[no]log_to_stderr       Do[n't] log to stderr
       -?, --help                Display this help text and exit
       -v, --version             Display the version of this binary and exit

    Examples
       $ evqld --standalone --datadir /var/evql
       $ evqld -c /etc/evqld.conf --daemonize --pidfile /run/evql.pid

The EventQL Server also optionally reads a set of configuration variables that
affect its operation as it runs. Configuration variables can be set as command
line flags or loaded from a configuration file. For more information on the
support configuration variables check the Configuration page.



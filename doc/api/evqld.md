4.2 evqld Reference
==================

evqld is the command line tool to start a server in an existing cluster.

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
                                                           
    Examples:                                              
       $ evqld --standalone --datadir /var/evql
       $ evqld -c /etc/evqld.conf --daemonize --pidfile /run/evql.pid

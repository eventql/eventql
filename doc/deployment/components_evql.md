2.3.2 evql
==========

The evql program is a simple command line utility that executes SQL and MapReduce
queries on an EventQL server. It supports an interactive shell with line editing
capabilities and noninteractive use.

When used interactively, query results are presented in an ASCII-table format.
When used noninteractively (`-B` flag), the result is presented in a tab-separated
format.

The EventQL command lines connects directly to the EventQL server.

You can use the EventQL command line to create tables, alter tables, insert rows
and for executing SQL and MapReduce queries.

    $ evql --help

    Usage: $ evql [OPTIONS] [query]
           $ evql [OPTIONS] -f file

       -f, --file <file>         Read query from file
       -e, --exec <query_str>    Execute query string
       -l, --lang <lang>         Set the query language ('sql' or 'js')
       -D, --database <db>       Select a database
       -h, --host <hostname>     Set the EventQL server hostname
       -p, --port <port>         Set the EventQL server port
       -u, --user <user>         Set the auth username
       --password <password>     Set the auth password (if required)
       --auth_token <token>      Set the auth token (if required)
       -B, --batch               Run in batch mode (streaming result output)
       --history_file <path>     Set the history file path
       --history_maxlen <len>    Set the maximum length of the history
       -q, --quiet               Be quiet (disables query progress)
       --verbose                 Print debug output to STDERR
       -v, --version             Display the version of this binary and exit
       -?, --help                Display this help text and exit

    Examples:
       $ evql                        # start an interactive shell
       $ evql -h localhost -p 9175   # start an interactive shell
       $ evql < query.sql            # execute query from STDIN
       $ evql -f query.sql           # execute query in query.sql
       $ evql -l js -f query.js      # execute query in query.js
       $ evql -e 'SELECT 42;'        # execute 'SELECT 42'


#### Default host, port and user

By default evql connects to the server `localhost` at port `9175` and set user to
the current UNIX user.

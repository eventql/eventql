4.1 evql Reference
==================


evql is the EventQL command line interface to create tables, alter tables, insert and query the data.

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


####Host, port and user
By default evql connects to the server `localhost` at port `9175` and set user to the current UNIX user.
You can easily change it by using the command line options.

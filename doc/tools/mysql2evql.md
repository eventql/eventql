8.1 mysql2evql
==============

mysql2evql is a command line tool that allows you to import a MySQL table into
EventQL. Before you can import your MySQL table, you need to create the
corresponding table in EventQL with a compatible schema.


    Usage: $ mysql2evql [OPTIONS]

       --source_table            Name of the MySQL table to import
       --destination_table       Name of the EventQL table to import to
       -D, --database <db>       Select a database
       -h, --host <hostname>     Set the EventQL server hostname
       -p, --port <port>         Set the EventQL server port
       --auth_token <token>      Set the auth token (if required)
       -x, --mysql               MySQL connection string
       --filter                  Boolean SQL expression to filter the import table data
       --shard_size              Size of a shard
       --upload_threads          Number of concurrent threads


    Examples:
      mysql2evql --source_table users --destination_table users --mysql "mysql://localhost:3306/my_db?user=root" --host localhost --port 9175 --database all

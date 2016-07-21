4.4 mysql2evql Reference
==================

mysql2evql is the EventQL command line tool to import a MySQL table to an existing EventQL.
Before you can import your MySQL table, you need to create the corresponding EventQL table with the same table schema than the initial table.


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
       

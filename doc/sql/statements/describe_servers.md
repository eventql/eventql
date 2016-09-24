5.2.9 DESCRIBE SERVERS
===============================

    DESCRIBE SERVERS



Displays information about the servers of the cluster.


####Example

    evql> DESCRIBE SERVERS;

    +---------------+---------------+-----------------+---------------+---------------+---------------+---------------+---------------+
    | Name          | Status        | ListenAddr      | BuildInfo     | Load          | Disk Used     | Disk Free     | Partitions    |
    +---------------+---------------+-----------------+---------------+---------------+---------------+---------------+---------------+
    | node1         | SERVER_UP     | localhost:10001 | v0.5.0-dev () | 0.204871      | 1609MB        | 6247MB        | 311/311       |
    | node3         | SERVER_UP     | localhost:10003 | v0.5.0-dev () | 0.206247      | 1621MB        | 6241MB        | 319/319       |
    | node2         | SERVER_UP     | localhost:10002 | v0.5.0-dev () | 0.204448      | 1604MB        | 6241MB        | 314/314       |
    | node4         | SERVER_UP     | localhost:10004 | v0.5.0-dev () | 0.26105       | 2206MB        | 6246MB        | 316/316       |
    +---------------+---------------+-----------------+---------------+---------------+---------------+---------------+---------------+


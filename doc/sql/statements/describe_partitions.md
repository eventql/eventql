5.2.7 DESCRIBE PARTITIONS
===============================

    DESCRIBE PARTITIONS table_name



DESCRIBE PARTITIONS displays information about all partitions of the table.


Example

    evql> DESCRIBE PARTITIONS sensors;

    +------------------------------------------+------------------------+---------------------+
    | Partition id                             | Keyrange Begin         | Servers             |
    +------------------------------------------+------------------------+---------------------+
    | 48127ec1a9c0812af6ba0240013c99baea40e22e | 2016-02-08 00:00:00    | node1, node3, node4 |
    +------------------------------------------+------------------------+---------------------+


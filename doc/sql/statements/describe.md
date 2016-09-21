5.2.7 DESCRIBE
==============

    DESCRIBE table_name

    DESCRIBE PARTITIONS table_name


DESCRIBE PARTITONS provides information about the partitions of a table.
For each partition the partition id, the list of servers that hold this partition
and the begin of the keyrange is returned.


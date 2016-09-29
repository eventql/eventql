3.7 Updating Rows
=================

EventQL supports updating a row/event after it was written. Like inserts,
updates are immediately visible once they are executed.

### Updating full rows (UPSERT)

To update a row, simply perform a new insert with the exact same primary key.
All rows are unique by primary key and two consecutive inserts with the same
primary key value are treated as an insert followed by an update (that is,
every insert with a primary key value equal to that of another row that already
exists will replace that original row).

#### Example:

    evql> CREATE TABLE my_events (event_id STRING PRIMARY KEY, value STRING);

    evql> INSERT INTO my_events (event_id, value) VALUES ("myid", "before");

    evql> SELECT * from my_events;
    =========================
    | event_id    | value   |
    =========================
    | myid        | before  |
    =========================

    evql> INSERT INTO my_events (event_id, value) VALUES ("myid", "after");

    evql> SELECT * from my_events;
    =========================
    | event_id    | value   |
    =========================
    | myid        | after   |
    =========================


### Updating individual fields (UPDATE ... SET field=value)

Currently the only supported update operation is replacing a full row. Updating
individual fields of a row is a feature that will be added in the future.

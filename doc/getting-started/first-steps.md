1.4 First Steps
===============

This page will guide you through setting up a standalone EventQL server, creating
a table, inserting a few events and finally querying the data.

If you want to learn how to set up a multi-machine cluster, jump straight to
["Setting up a new cluster"](../../cluster-administration/cluster-setup).

### Step 1: Start the EventQL server

We'll begin by starting a standalone EventQL server. If you already have a running
EventQL cluster or are an EventQL Cloud customer, you can skip this step.


The commands below will start an evqld process that stores its database in
`/var/evql/standalone`.

    $ mkdir -p /var/evql/standalone
    $ evqld --cachedir /tmp --config_backend standalone --client_auth_backend trust --listen localhost:9175 --datadir /var/evql/standalone

The server should now be running and listening on `localhost:9175`. Note that we
didn't pass the `--daemonize` flag, so the server process is not sent into the
background.

Next, open a new terminal and copy the command below to start an interactive sql
shell:

    $ evql --database test

More documentation on how to install and run the server can be found in the
["Installation"](../installation/) and ["Running EventQL"](../running-eventql/)
pages.

### Step 2: Creating a new table

Similar to most SQL databases, the core units of data storage in EventQL are tables
and rows (row are also referred to as records or events). For our example, we'll
create a simple table that stores a clickstream/http server access log.

    create table test (time datetime, val uint64, primary key (time, val));

Note that EventQL can deal with more complex (nested) schemas that allow you
to store any JSON object into a row. Check out the
["Tables & Schemas"](../../collecting-and-storing/tables/) page for more
information.

### Step 3: Insert events

    insert into test (time, val) values (now(), 1337);


### Step 4: Query the data

    select * from test;

1.2 First Steps
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
    $ evqld --standalone --datadir /var/evql/standalone

The server should now be running and listening on `localhost:9175`. Note that we
didn't pass the `--daemonize` flag, so the server process is not sent into the
background.

Next, open a new terminal and copy the command below to start an interactive sql
shell:

    $ evql --database test

More documentation on how to install and run the server can be found in the
["Installation"](../installation/) and ["Running EventQL"](../running-eventql/)
pages.

### Step 2: Create a new table

Similar to most SQL databases, the core units of data storage in EventQL are tables
and rows (rows are also referred to as records or events). For our example, we'll
create a simple table that stores a http server access log.

Our table will be called `access_log` and will contain three columns: `time`,
`session_id` and `url`. We will use the combination of time and session_id
as our primary key.

To choose a good primary key and get the best performance, it is important
to understand how data is partitioned and distributed across machines. Read more
about primary keys on the ["Partitioning" page](../../collecting-and-storing/tables/partitioning).

Copy the command below into the SQL shell that we started in the previous step
to create the table:

    CREATE TABLE access_log (
      time        DATETIME,
      session_id  STRING,
      url         STRING,
      PRIMARY KEY (time, session_id)
    );

Note that EventQL can deal with more complex (nested) schemas that allow you
to store any JSON object into a row. Check out the
["Tables & Schemas"](../../collecting-and-storing/tables/) page for more
information.

### Step 3: Insert events

Now we are ready to start inserting events into the `access_log` table. For
our example, execute the SQL commands below a couple of times to create a bit
of test data:

    INSERT INTO access_log (time, session_id, url) VALUES (NOW(), "s1", "/page1");
    INSERT INTO access_log (time, session_id, url) VALUES (NOW(), "s2", "/page2");
    INSERT INTO access_log (time, session_id, url) VALUES (NOW(), "s3", "/page1");

Later, we will use the [HTTP API](../../api/http/) or one of the [driver libraries](../../api/)
to insert records programatically.

### Step 4: Query the data

Now let's query the data we just inserted. The most simple query returns all rows
in the table:

    SELECT * FROM access_log;

EventQL aims to be a feature-complete SQL database. We're not there just yet, but
at this point have most of the standard statements and functions you'd expect (yes,
including JOINs).

For a more complex query example, let's display the top ten pages in the last 4
hours:

     SELECT url, count(1) cnt
     FROM access_log
     GROUP BY url
     ORDER BY cnt DESC
     LIMIT 10;

That's all for now. To dive deeper, check out the ["SQL Query Language" chapter](../../queries/sql/introduction/)
for more information on all supported SQL statements, functions and extensions
or have a look at the [JavaScript Query API](../../queries/pipelines/mapreduce/)

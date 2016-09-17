4.1 Timeseries & Logs
=====================

EventQL is an excellent fit to store high volumes of timeseries-structured data
like metrics, tracking data, logfiles, sensor measurements or other
machine-generated data.

*To get the best performance when handling large arrival rates of new data, please
see the "Settings for High Volume Timeseries Data" section below.*

### Timeseries Queries

When you have set up the table as described below, EventQL can perform efficient
scans on time ranges. For example, this query will only have to read the subset
of data that was written in the last 24 hours, regardless of the total table
size:

    SELECT time, event_id, ...
    FROM high_volume_logging_data
    WHERE time > time_at("-24h") and time < time_at("now");

Using the ChartSQL extensions, you can quickly get a chart of some data, for
example here is a query that will display a plot of the number of written events
per minute for the last 3 days:


    DRAW LINECHART AXIS LEFT AXIS BOTTOM;

    SELECT count(1) y, date_trunc("1m", time) x
    FROM high_volume_logging_data
    WHERE time > time_at("-3d")
    GROUP BY date_trunc("1m", time)
    ORDER BY time desc;

This is what the output should look like (using the eventql-console web
application):

<img style="width: 100%;" src="example_timeseries_chart.png" />


Settings for High Volume Timeseries Data
----------------------------------------

To get the best performance when handling large arrival rates of new events,
we recommend these settings:

#### 1. Use a datetime field as the partitioning key

You should use a datetime field as the first part of the primary key for time
series tables. Since primary keys are automatically unique, the usual pattern
when setting up a timeseries table is to use a primary key that consists of the
event time and a unique event id.

#### 2. Set the partition size hint

EventQL will dynamically re-partition the table as you add more data to keep
each partition in the 500MB to 1GB range. You can optionally give EventQL a hint
about the amount of writes you expect. Specifying this hint will minimize the
number of re-partitioning operations and allow you to ramp up the insert rate
much quicker.

To set the hint you have to specifty a `finite_partition_size` when creating a
table.

    CREATE TABLE high_volume_logging_data (
      time            DATETIME,
      event_id        STRING,
      ...
      PRIMARY KEY(time, event_id)
    ) WITH finite_partition_size = 600000000;

Use this formula to calculate the value:

    $finite_partition_size = (750MB / $expected_new_data_per_day) * 86400000000

So, for example, if you expect around 10GB of new data a day, `7200000000` would be
a good value. If you expect 1000GB of new data a day, `60000000` is a good value.
If you expect 10TB of new data a day, set the finite partition size to `10000000`.

If your estimation is off, it will not cause any problems. You can update the
hint at any time. To read more about the finite partition size setting check out
the [Partitioning page] (../../tables/partitioning/).

#### 3. Enable the async_split option

Another recommended setting for handling high volumes of inserts is enabling the
`enable_async_split` switch. This allows a splitting partition to immediately
redirect all inserts to the new targets once it has started the split.

To apply the async split option to a table, you can use this simple SQL
statement

    ALTER TABLE high_volume_logging_data SET PROPERTY enable_async_split="true";

To read more about the finite partition size setting check out the
[Table Options page] (../../tables/table-options/).



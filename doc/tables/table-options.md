3.8 Table Options
=================

This page lists all configuration values that you can optionally set on a table.
To set a configuration value, you can either use the `CREATE ... WITH key=val` or
the `ALTER TABLE SET PROPERTY key=val` SQL statements or the HTTP API.

<table style="font-size:90%;">
  <tr>
    <th>Option</th>
    <th>Default Value</th>
    <th>Description</th>
  </tr>
  <tr>
    <td><b>partition_size_hint</b></td>
    <td>NULL</td>
    <td>
      <p>
        When set, enables hinted partitioning for a table. See the
        <a href="../partitioning/"> partitioning page</a> and
        <a href="../../collecting-data/high-volume-timeseries-logs/">timeseries page</a>
        for more details. The value of the option is an integer (the partition size).
        For timeseries tables the integer is a microsecond time duration.
      </p>
      <p>
        Note that while the partition_size_hint can be changed at any time, it
        can only be initially set when creating a table. I.e. when a table was
        created without a partition size hint it can't be later added. When a
        table was created with a partition size hint the hint may be updated at
        any time.
      </p>
    </td>
  </tr>
  <tr>
    <td><b>enable_async_split</b></td>
    <td>false</td>
    <td>
      <p>
        If set to true, allow partition splits to complete immediately before
        the data has been fully replicated. In the normal mode (false) a partition
        split will only become visible/active after at least a majority of the new
        servers have acknowledged all data in the splitting partition.
      </p>
      <p>
        This an be problematic with large volumes of inserts as -- until the
        split has finished -- all inserts will still go into the splitting
        partition. So the splitting partition has to complete the replication
        and handle all inserts at the same time before it can divide the load
        onto the new partitions.
      </p>
      <p>
        If you're inserting high volumes of data setting this value to true
        is recommended as it allows a splitting partition to immediately
        redirect all inserts to the new targets once it has started the split.
        This allows you to ramp up the insert rate for a given keyrange much
        quicker. The downside of setting the value however is that it becomes
        more likely that you'll temporarily see a stale view of the data while
        the split is running (also see the notes on eventual consistency in the
        architecture documentation).
      </p>
    </td>
  </tr>
  <tr>
    <td><b>partition_split_threshold</b></td>
    <td>512MB</td>
    <td>
      <p>
        The partition_split_threshold value controls above what size a partition
        will be considered too large and subdivided into smaller partitions. It
        works out so that every partition will be between `0` and
        `partition_split_threshold * 2` bytes large. The value must be passed as
        an integer number of bytes.
      </p>
      <p>
        This setting may be changed at any time. It's not recommended to change
        the default value unless you have specific reason to do so.
      </p>
    </td>
  </tr>
  <tr>
    <td><b>disable_split</b></td>
    <td>false</td>
    <td>
      If set to true, prevent all partitions in the table from splitting. This
      setting may be changed at any time. It's not recommended to change the
      default value unless you have specific reason to do so.
    </td>
  </tr>
  <tr>
    <td><b>disable_replication</b></td>
    <td>false</td>
    <td>
      If set to true, disable replication for the table. This setting may be
      changed at any time. It's not recommended to change the default value
      unless for debugging/trubleshooting purposes.
    </td>
  </tr>
</table>








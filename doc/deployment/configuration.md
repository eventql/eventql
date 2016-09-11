2.2 Configuration
=================

Configuration files aren't mandatory but they provide a convenient way to specify options that you use regularly when running EventQL programs, so you don't have to enter them on the command line each time you run the program.

### Configuration File Format
EventQL uses the INI file format, that is a simple text file grouped into sections that consists of key-value pairs.

    [client]
    host = prod.example.com ; execute queries on this server

You can override every option set in the configuration file by using the command line option `-C` followed by the corresponding `section.key=value` pair.

    $ evql -C client.host=localhost



### Configuration File Path

If no explicit config path is provided using the `--config` option, EventQL will
search for the configuration file at the following locations:

<table style="font-size:90%;">
  <tr>
    <th>Binaries</th>
    <th>Config Search Paths</th>
  </tr>
  <tr>
    <td>evqld</td>
    <td>/etc/evqld.conf</td>
  </tr>
  <tr>
    <td>evql, evqlctl</td>
    <td>/etc/evql.conf<br/>~/.evql.conf</td>
  </tr>
</table>


### Configuration Options

The EventQL configuration options are grouped in three sections: `client`, `server` and `cluster`.

<table style="font-size:90%;">
  <tr>
    <th>Option</th>
    <th>Default Value</th>
    <th>Description</th>
  </tr>
  <tr>
    <th colspan="3" align="left">cluster.*</th>
  </tr>
  <tr>
    <td><b>cluster.name</b></td>
    <td>&mdash;</td>
    <td>The name of the cluster</td>
  </tr>
  <tr>
    <td><b>cluster.coordinator</b></td>
    <td>&mdash;</td>
    <td>The cluster coordinator service. Legal values: "zookeeper"</td>
  </tr>
  <tr>
    <td><b>cluster.zookeeper_hosts</b></td>
    <td>&mdash;</td>
    <td>A comma-separated list of zookeeper hosts (only used when cluster.coordinator=zookeeper)</td>
  </tr>
  <tr>
    <td><b>cluster.rebalance_interval</b></td>
    <td>60000000</td>
    <td></td>
  </tr>
  <tr>
    <td><b>cluster.allowed_hosts</b></td>
    <td>&mdash;</td>
    <td>
      A comma-separated list of CIDR network ranges that are allowed to
      connect as internal nodes to the cluster. This setting does not affect
      which hosts are allowed to connect as a client. You can set this option
      to "0.0.0.0/0" to allow all hosts to connect as internal nodes.
    </td>
  </tr>
  <tr>
    <th colspan="3" align="left">server.*</th>
  </tr>
  <tr>
    <td><b>server.datadir</b></td>
    <td>&mdash;</td>
    <td>The location of the EvenQL data directory (mandatory)</td>
  </tr>
  <tr>
    <td><b>server.listen</b></td>
    <td>&mdash;</td>
    <td>
      The address (host:port) on which the server should listen. NOTE that this
      address is published to the coordinator service and must be a reachable 
      by all other servers in the cluster. I.e. you can't use localhost or
      0.0.0.0. (mandatory)
    </td>
  </tr>
  <tr>
    <td><b>server.name</b></td>
    <td>&mdash;</td>
    <td>The name of the server (optional)</td>
  </tr>
  <tr>
    <td><b>server.pidfile</b></td>
    <td>&mdash;</td>
    <td>
      If set, the server will write a pidfile to the provided path and aquire
      an exclusive lock on the pidfile. If the exclusive lock fails, the server
      will exit.
    </td>
  </tr>
  <tr>
    <td><b>server.daemonize</b></td>
    <td>false</td>
    <td></td>
  </tr>
  <tr>
    <td><b>server.indexbuild_threads</b></td>
    <td>2</td>
    <td>The number of background compaction threads to start</td>
  </tr>
  <tr>
    <td><b>server.client_auth_backend</b></td>
    <td>&mdash;</td>
    <td></td>
  </tr>
  <tr>
    <td><b>server.internal_auth_backend</b></td>
    <td>&mdash;</td>
    <td></td>
  </tr>
  <tr>
    <td><b>server.noleader</b></td>
    <td>false</td>
    <td></td>
  </tr>
  <tr>
    <td><b>server.gc_mode</b></td>
    <td>AUTOMATIC</td>
    <td></td>
  </tr>
  <tr>
    <td><b>server.gc_interval</b></td>
    <td>30000000</td>
    <td></td>
  </tr>
  <tr>
    <td><b>server.cachedir_maxsize</b></td>
    <td>68719476736</td>
    <td>Unit: Bytes</td>
  </tr>
  <tr>
    <td><b>server.c2s_io_timeout</b></td>
    <td>1s</td>
    <td>
      How long should the server wait for data on a connection to a client when
      it expects the data to arrive immediately. (optional, unit: microseconds)
    </td>
  </tr>
  <tr>
    <td><b>server.c2s_idle_timeout</b></td>
    <td>30min</td>
    <td>
      How long should the server wait for new data on an idle connection to a
      client. An idle connection is a connection where no data is expected
      to arrive immediately. (optional, unit: microseconds)
    </td>
  </tr>
  <tr>
    <td><b>server.s2s_io_timeout</b></td>
    <td>1s</td>
    <td>
      How long should the server wait for data on a connection to another
      server when it expects the data to arrive immediately. (optional,
      unit: microseconds)
    </td>
  </tr>
  <tr>
    <td><b>server.s2s_idle_timeout</b></td>
    <td>5s</td>
    <td>
      How long should the server wait for new data on an idle connection to
      another server. An idle connection is a connection where no data is
      expected to arrive immediately. (optional, unit: microseconds)
    </td>
  </tr>
  <tr>
    <td><b>server.heartbeat_interval</b></td>
    <td>1s</td>
    <td>
      How often should the server send a keepalive/heartbeat frame on a busy
      connection. Note that this value must be lower than the idle timeout and
      also puts a lower limit on the idle timeout that a connection client may
      choose. (optional, unit: microseconds)
    </td>
  </tr>
  <tr>
    <th colspan="3" align="left">client.*</th>
  </tr>
  <tr>
    <td><b>client.host</b></td>
    <td>localhost</td>
    <td>The hostname of the EventQL server</td>
  </tr>
  <tr>
    <td><b>client.port</b></td>
    <td>9175</td>
    <td>The port of the EventQL server</td>
  </tr>
  <tr>
    <td><b>client.database</b></td>
    <td>&mdash;</td>
    <td>The database that should be used for following queries (optional)</td>
  </tr>
  <tr>
    <td><b>client.user</b></td>
    <td>&mdash;</td>
    <td>Username to use when connecting to server (optional)</td>
  </tr>
  <tr>
    <td><b>client.password</b></td>
    <td>&mdash;</td>
    <td>Password to use when connecting to server (optional)</td>
  </tr>
  <tr>
    <td><b>client.auth_token</b></td>
    <td>&mdash;</td>
    <td>Auth-Token to use when connecting to server (optional)</td>
  </tr>
</table>




2.2 Configuration
=================

Configuration files aren't mandatory but they provide a convenient way to specifiy options that you use regurlay when running EventQL programs, so you don't have to enter them on the command line each time you run the program.

###File Format
EventQL uses the INI file format, that is a simple text file grouped into sections that consists of key-value pairs.

    [client]
    host = prod.example.com ; execute queries on this server

You can override every option set in the configuration file by using the command line option `-C` followed by the corresponding `section.key=value` pair.

    $ evql -C client.host=localhost



###File path

If not set explicitely with the --config option, EventQL will search for the configuration
file at the following locations:


#### /etc/evqld.conf
&nbsp;&nbsp;&nbsp;&nbsp; evqld configuration file

#### /etc/evql.conf
&nbsp;&nbsp;&nbsp;&nbsp; evql and evqlctl configuration file

#### ~/.evql.conf
&nbsp;&nbsp;&nbsp;&nbsp; evql and evqlctl configuration file, overwrites options from
/etc/evql.conf




###Configuration options
The EventQL configuration options arw grouped in three sections: `client`, `server` and `cluster`.

###client

**host** <br>
&nbsp;&nbsp;&nbsp;&nbsp;(Default: localhost) The IP address or hostname to send the query to.

**port**<br>
&nbsp;&nbsp;&nbsp;&nbsp;(Default: 9175) The host's port.

**database**<br>
&nbsp;&nbsp;&nbsp;&nbsp;The database that should be used for following statements.

**user**<br>
&nbsp;&nbsp;&nbsp;&nbsp;The current user's username.

**password**<br>
&nbsp;&nbsp;&nbsp;&nbsp;The current user's password.

**auth\_token**<br>
&nbsp;&nbsp;&nbsp;&nbsp;


###server
**name**<br>
&nbsp;&nbsp;&nbsp;&nbsp;The name of the server.

**datadir**<br>
&nbsp;&nbsp;&nbsp;&nbsp;The location of the EvenQL data directory.

**listen**<br>
&nbsp;&nbsp;&nbsp;&nbsp;The address (host:port) the server listens to.

**indexbuild\_threads**<br>

**client\_auth\_backend**<br>
&nbsp;&nbsp;&nbsp;&nbsp;Must be either `trust` or `legacy`.

**legacy\_auth\_secret**<br>

**pidfile**


###cluster
***name***<br>
&nbsp;&nbsp;&nbsp;&nbsp;The name of the cluster.

***coordinator***<br>
&nbsp;&nbsp;&nbsp;&nbsp;The cluster coordinator service, e.g. zookeeper.

***zookeeper\_hosts***<br>
&nbsp;&nbsp;&nbsp;&nbsp;A comma-separated list of zookeeper hosts.


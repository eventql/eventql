4.1 Configuration File
=====================

Configuration files aren't mandatory but they provide a convenient way to specifiy options that you use regurlay when running EventQL programs, so you don't have to enter them on the command line each time you run the program.

You can override every option set in the configuration file by using the command line option `-C` followed by the corresponding `section.key=value` pair.

    $ evql -C client.host=localhost

###File Format
EventQL uses the INI file format.

    [evql]
    host = prod.example.com ; execute queries on this server


###File path
By default EventQL tries to read the configuration from `/etc/{process}.conf` and `~/.evqlrc`.
You can specify a custom file path to read your configuration from with the command line option `-c file_path`.


###Configuration options
The EventQL configuration options arw grouped in three sections: `client`, `server` and `cluster`.

###client

**host** <br>
&nbsp;&nbsp;&nbsp;&nbsp;(Default: localhost) The IP address or hostname to send the query to.

**port**<br>
&nbsp;&nbsp;&nbsp;&nbsp;(Default: 9175) The port for the host.

**database**<br>
&nbsp;&nbsp;&nbsp;&nbsp;The database that should be used for following statements.

**user**

**password**

**auth_token**


###server
**name**

**datadir**

**listen**

indexbuild_threads

client_auth_backend

legacy_auth_secret

deamonize

pidfile

gc_mode


###cluster
***coordinator***

name


###evqlctl
cluster_name

server_name

master

table_name

namespace

partition_id

split_point

primary_key



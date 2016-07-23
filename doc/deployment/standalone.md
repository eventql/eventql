2.4 Standalone Mode
===========

While EventQL is a distributed database first and foremost it also implements a
"standalone mode". In the standalone mode evqld runs as a standalone process on
a single machine and has no further dependencies. The standalone mode is ideal
for evaluating and quickly get EventQL running on your development machine. You
can have a standalone EventQL server up and running in seconds.

To start a server in standalone mode all you need to provide is a data directory.
The commands below will start a server process that stores its database in
`/var/evql/standalone`.

    $ mkdir -p /var/evql/standalone
    $ evqld --standalone --datadir /var/evql/standalone


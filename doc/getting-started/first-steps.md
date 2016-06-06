1.4 First Steps
===============

# start the eventql server

  $ mkdir -p /var/evql/standalone
  $ evqld --cachedir /tmp --config_backend standalone --client_auth_backend trust --listen localhost:9175 --datadir /var/evql/standalone

# start the eventql sql console

    $ evql --database test


# create a new table

    create table test (time datetime, val uint64, primary key (time, val));


# insert first record

    insert into test (time, val) values (now(), 1337);

# run first query

    select * from test;

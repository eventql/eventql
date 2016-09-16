Testing
=======

The EventQL test suite is separated into three parts: Unit Tests, System Tests
and Cluster Tests. You can either run the full test suite or the "smoke test
suite". The smoke test suite is a small subset of important unit and system
tests from the full test suite that is designed to run quickly.

Use the following scripts to invoke the respective test suites:

    test/run_full_test.sh                # Run the full test suite
    test/run_smoke_test.sh               # Run the smoke test suite

    test/run_unit_tests.sh               # Run all unit tests
    test/run_system_tests.sh [target]    # Run all system tests on a target server/cluster
    test/run_cluster_tests.sh            # Run all cluster tests


Note that the test suite scripts expects the test drivers and tests to be built.
To build the tests, you can use make:

    $ cd test && make

You can also use make to run the test suite

    $ make test                          # Run the full test suite
    $ make test-smoke                    # Run the smoke test suite


### Unit Tests

Unit tests are built as standalone programs that link directly against the core
EventQL codebase. Unit tests check the function of internal classes and other
units.

Examples for unit tests are "does the SQL substr() function return the
correct result" or "does the file descriptor cache work correctly".

The unit tests are stored in `test/unit`.


### System Tests

System tests are also built as standalone programs. However, they don't link to
the internal codebase, but use the public client libraries. The system tests
connect to a EventQL server or cluster and execute a series of commands using
the reqular client APIs, checking the results.

Examples for system tests are "insert 1000 rows and then check that queries
return the correct result" or "check that creating, droping and then re-creating
a table works".

The system tests are stored in `test/system`.


### Cluster Tests

The cluster tests are intended to test behaviour that is specific to a
distributed environment and the state of servers within the cluster. Like system
tests, cluster tests are built as standalone programs but they do not connect to
an existing EventQL server or cluster. Instead each cluster test spawns a number
of evqld processes and a cluster coordinator itself and then uses a mix of
client APIs and access to the internal data structures of each server to verify
distributed behaviour.

A simple example for a cluster test is "does a given query still work if n-1 of
the replicas are down?"

The cluster tests are stored in `test/cluster`.


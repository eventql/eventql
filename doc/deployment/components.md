2.3 Components
==============

The EventQL distribution contains three programs/binaries called `evql`
(client and interactive shell), `evqld` (server) and `evqlctl` (cluster control).


### [evql](./evql/) - EventQL Shell &amp; Command Line

The evql program is a simple command line utility that executes SQL and MapReduce
queries on an EventQL server. It supports an interactive shell with line editing
capabilities and noninteractive use.


### [evqld](./evqld/) - EventQL Server

evqld, also known as the EventQL server, is the main server program. When the
EventQL server starts, it listens for network connections and HTTP requests from
client programs and executes writes and queries on behalf of those clients.

### [evqlctl](./evqlctl/) - EventQL Cluster Administration

evqlctl is a client command line utility for performing administrative operations
on an EventQL server or cluster. You can use it to check and change a clusters
configuragtion and current status, to add and remove servers and more.



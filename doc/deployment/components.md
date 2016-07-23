2.3 Components
==============

The EventQL distribution contains three programs/binaries called `evql`
(client and interactive shell), `evqld` (server) and `evqlctl` (cluster control).


### evql


### [evqld](./evqld/)

evqld, also known as the EventQL server, is the main server program. When the
EventQL server starts, it listens for network connections and HTTP requests from
client programs and executes inserts and queries on behalf of those clients.


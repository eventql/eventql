1.5 FAQ
=======

#### Is EventQL a key-value store?


#### Why does EventQL require a strict schema? // Why isn't EventQL schemaless?

So why does EventQL require a strict schema for all tables? Would a schemaless
approach not have been easier to use? &mdash; There are a couple of important
reasons why we need to have strict schemas in EventQL:

The most important reason is that EventQL is a columnar database, which
means that it stores data tables as columns rather than as rows. This allows us
to read only the required columns we need to answer a query rather than scanning
and discarding unwanted data from full rows which often leads to huge performance
increases for IO-bound queries on very large datasets.

Disassembling the records for columnar store requires a schema. While it would
have been possible to generate a schema on the fly when building the columnar
tables this would have meant a lot of negative implications on performance, IO
load and the replication architecture.

Furthermore, having a strict schema enables a lot of optimizations and diagnostics
in the SQL engine that would have been much harder or impossible to build with
an adaptive/ad-hoc schema approach.

#### What kind of consistency guarantees does EventQL provide?

#### Does EventQL support transactions?

#### Why AGPLv3 and not MIT/BSD?

Many man-years of research and work have gone into EventQL. Since all of the
contributors need to pay rent and feed their families the project frankly
wouldn't exist if there was no way for us to earn money around it.

However, we're deeply comitted to open source and make a pinkie promise that
we will never move the project into a closed/propietary direction. We understand
that we are deeply dependant on the trust and goodwill of the open source community.
If anything, we will reconsider to make the license more permissible in the future.

#### When should I use EventQL Platform?

#### When should I not use EventQL Platform?

#### Is EventQL schemaless?

#### What are some example use cases for EvetQL?

#### Which drivers and languages can connect to EventQL?
EventQL has a MySQL compatibility layer so any tool or language that can already connect to MySQL can also connect to EventQL.

Additionally we offer native drivers in C, C++, Java, Python, Ruby, Go, Rust and JavaScript. The native drivers implement some additional features like query progress indication and improved serialization for nested records.

#### How does EventQL compare to Hadoop?

#### How does EventQL compare to MySQL/PostgreSQL

#### Can I use BI Tools like Tableau with EventQl?

EventQL supports visualization tools like Tableau through a MySQL compatiblity layer. You can use standard the standard drivers for your tool and language (JDBC/ODBC). Any BI tools that can already connect to MySQL can also be used with EventQL.


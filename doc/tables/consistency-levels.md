3.x Consistency Levels
======================

Write Consistency Level
-----------------------

all writes are always eventually consistent. the consistency level mainly
controls a tradeoff between timelieness (i.e. how quickly a new write is visible
on all servers) vs performance.

    STRICT       -- offers the best possible consistency in every scenarion at the cost of performance (DEFAULT)
    RELAXED      -- write is guaranteed to be durable, but might take some time to become visible. offers the best performance
    BEST_EFFORT  -- accept a write into a non-owning replica and writes into a single replica. offers best availability but writes may take a long time to appear and get lost in some corner cases


Query Consistency Level
-----------------------

the consistency level mainly controls a tradeoff between timelieness (i.e. if
the query picks up all recent writes) vs performance.

    STRICT       -- try to get the most recent value in all tables at the cost of performance (DEFAULT)
    RELAXED      -- query result is guaranteed to be consistent but might not include some recent inserts/writes. offers the best performance


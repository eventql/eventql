### date_trunc

Truncates a DateTime value to the specified window/precision. Given an input
timestamp this function returns the start time of the time window that contains
the input timestamp.

    date_trunc(window, timestamp)

The window parameter must be a string and must be equal to or end with one of the
time units below. Optionally, you can prefix the time unit with an integral number
to make the window a multiple of the unit.

The valid time units are:

    ms/msec/millisecond/milliseconds
    s/sec/second/seconds
    min/minute/minutes
    h/hour/hours
    d/day/days
    w/week/weeks
    month/months
    y/year/years

Examples:

    SELECT date_trunc('hour', TIMESTAMP '2001-02-16 20:38:40');
    Result: 2001-02-16 20:00:00

    SELECT date_trunc('30mins', TIMESTAMP '2001-02-16 20:38:40');
    Result: 2001-02-16 20:30:00

    SELECT date_trunc('year', TIMESTAMP '2001-02-16 20:38:40');
    Result: 2001-01-01 00:00:00



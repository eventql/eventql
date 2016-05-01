### time_at
Returns a DateTime value for a time value or a time interval from now.

Time values can be a unix timestamp, the literal 'now' or a time string in the
format %Y-%m-%d %H:%M:%S

```
SELECT TIME_AT('1451910364')
--> '2016-01-04 12:26:04'
```


Interval values can be written using the following syntax:

```
  [sign]<quantity><unit> [direction]
```

where *sign* can be '-' or empty, *direction* can be 'ago' or empty but either sign or
direction have to be given, *quantity* is an integer and *unit* is one of the following
possible time units:

    s/sec/secs/second/seconds
    min/mins/minute/minutes
    h/hour/hours
    d/day/days
    w/week/weeks
    month/months
    y/year/years


```
SELECT TIME_AT('-7days')
SELECT TIME_AT('7days ago')
```
This would return the DateTime value for the current timestamp - 7 days.


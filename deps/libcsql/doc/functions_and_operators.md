Functions and Operators
=======================

Z1 provides a large number of functions and operators for the built-in data types.
Users can also define their own functions and operators.

---

###### Control Flow Functions
[if](#if_expression_)

###### String Functions
[REGEXP](#regexp-operator-),
[startswith](#startswith),
[endswith](#endswith),
[uppercase](#uppercase),
[ucase](#ucase),
[lowercase](#lowercase),
[lcase](#lcase)

###### Numeric Functions
[+](#add-operator),
[-](#sub-operator),
[*](#mul-operator),
[/](#div-operator),
[%](#mod-operator),
[pow](#pow),
[mod](#mod),
[round](#round),
[truncate](#truncate)

###### Boolean Functions
[!](#neg-operator),
[==](#eq-operator),
[!=](#neq-operator),
[<](#lt-operator),
[<=](#lte-operator),
[>](#gt-operator),
[>=](#gte-operator),
[&&](#and),
[||](#or),
[AND](#and),
[OR](#or)
[ISNULL](#isnull)

###### DateTime Functions
[from_timestamp](#from_timestamp),
[date_trunc](#date_trunc)
[date_add](#date_add)
[time_at](#time_at)

###### Aggregate Functions
[sum](#sum),
[count](#count),
[min](#min),
[max](#max),
[mean](#mean)

###### Conversion Functions
[to_str](#to_str),
[to_int](#to_int),
[to_float](#to_float),
[to_bool](#to_bool)

---
##### Operator Precedence

Operator precedences are shown in the following list, from highest precedence to
the lowest. Operators that are shown together on a line have the same precedence.

    !
    - (unary minus), ~ (unary bit inversion)
    ^
    *, /, DIV, %, MOD
    -, +
    <<, >>
    &
    |
    = (comparison), <=>, >=, >, <=, <, <>, !=, IS, LIKE, REGEXP, IN
    BETWEEN, CASE, WHEN, THEN, ELSE
    NOT
    &&, AND
    XOR
    ||, OR



<br /><br />
Control Flow Functions
----------------------

---
### if (expression)
Executes one of two subexpressions based on the return value of a conditional
expression.

    if(cond_expr, true_branch, false_branch)

If `cond_expr` is, the `true_branch` expression will be executed and the result
returned. Otherwise the `false_branch` expression will be executed and the result
returned.

Examples:

    SELECT if(1 == 1, 23, 42);
    Result: 23

    SELECT if(1 == 2, 2 * 2, 4 * 4);
    Result: 16


<br /><br />
String Functions
----------------

---
### REGEXP (operator)
Matches the provided `value` against the provided `pattern`. Returns true if the
`pattern` matches the `value` and false otherwise.

    value REGEXP pattern

The pattern must be a POSIX compatible regular expression.


---
### startswith
Returns true if the `value` strings begins with the `prefix` string or both
strings areq equal and false otherwise.

    startswith(value, prefix)


---
### endswith
Returns true if the `value` strings ends with the `suffx` string or both strings
are equal and false otherwise.

    endswith(value, prefix)


---
### uppercase

Returns a copy of the provided `value` string with all characters converted to
upper case.

    uppercase(value)


---
### ucase

Alias for `uppercase`


---
### lowercase

Returns a copy of the provided `value` string with all characters converted to
lower case.

    lowercase(value)


---
### lcase

Alias for `lowercase`


<br /><br />
Numeric Functions and Operators
-------------------------------

---
### + (operator)
Returns the sum of the values `a` and `b`.

    a + b

---
### - (operator)

Returns the difference of the values `a` and `b`.

    a - b

---
### * (operator)
Returns the product of the values `a` and `b`.

    a * b

---
### / (operator)
Returns the quotient of the values `a` and `b`.

    a / b

---
### % (operator)
Returns the modulo of the values `a` and `b`.

    a % b

---
### pow

Returns `a` to the power of `b`.

    pow(a, b)


---
### mod

Returns the modulo of the values `a` and `b`.

    mod(a, b)


---
### round

Returns the number `x` rounded to `d` decimal places. `d` defaults to 0 if
not specified.

    round(x)
    round(x, d)


---
### truncate

Returns the number `x` truncated to `d` decimal places. `d` defaults to 0 if
not specified.

    truncate(x)
    truncate(x, d)



<br /><br />
Numeric Functions and Operators
-------------------------------

---
### ! (operator)
Returns false if the `expr` is true and false if the `expr` is true.

    !expr


---
### == (operator)
Returns true if the values `a` and `b` are equal.

    a == b


---
### != (operator)
Returns true if the values `a` and `b` are not equal.

    a != b


---
### < (operator)
Returns true if the values `a` is strictly less than `b`

    a < b

---
### <= (operator)
Returns true if the values `a` is strictly less than or equal to `b`

    a <= b

---
### > (operator)
Returns true if the values `a` is strictly greater than `b`

    a > b


---
### >= (operator)
Returns true if the values `a` is strictly greater than or equal to `b`

    a >= b

---
### && (operator)

Returns true if the values `a` and `b` are both true.

    a AND b


---
### || (operator)

Returns true if one or both of the values `a` and `b` are true.

    a || b


---
### AND

Returns true if the values `a` and `b` are both true.

    a AND b


---
### OR

Returns true if one or both of the values `a` and `b` are true.

    a OR b


---
### isnull

Returns true iff the value `a` is NULL and false otherwise. I.e. this method will return true only in one single case and that is  when the provided value has the valuetype NULL. This method will return false for other null-like values that are not the exact NULL valuetype, like the numeric zero or the empty string.

    isnull(value)




<br /><br />
DateTime Functions
------------------

---
### from_timestamp
Convert a numeric unix timestamp into a DateTime value.

    from_timestamp(timestamp)


---
### date_trunc

Truncates a DateTime value to the specified window/precision. Given an input
timestamp this function returns the start time of the time window that contains
the input timestamp.

    date_trunc(window, timestamp)

The window parameter must be a string and must be equal to or end with one of the
time units below. Optionally, you can prefix the time unit with an integral number
to make the window a multiple of the unit.

The valid time units are:

    s/sec/secs/second/seconds
    m/min/mins/minute/minutes
    h/hour/hours
    d/day/days
    w/week/weeks
    m/month/months
    y/year/years

Examples:

    SELECT date_trunc('hour', TIMESTAMP '2001-02-16 20:38:40');
    Result: 2001-02-16 20:00:00

    SELECT date_trunc('30mins', TIMESTAMP '2001-02-16 20:38:40');
    Result: 2001-02-16 20:30:00

    SELECT date_trunc('year', TIMESTAMP '2001-02-16 20:38:40');
    Result: 2001-01-01 00:00:00


---
### date_add
Adds an interval to a DateTime value.

    date_add(date, expr, unit)

The date argument indicates the starting DateTime or Timestamp value.
Expr is a string specifying the interval value to be added, it may start with a '-' for negative values.
Unit is a string specifying the expression's unit.

| Unit           | Expr Format                 |
| -------------- | --------------------------- |
| SECOND         | SECONDS                     |
| MINUTE         | MINUTES                     |
| HOUR           | HOURS                       |
| DAY            | DAYS                        |
| WEEK           | WEEKS                       |
| MONTH          | MONTHS                      |
| YEAR           | YEARS                       |
| MINUTE_SECOND  | MINUTES:SECONDS             |
| HOUR_SECOND    | HOURS:MINUTES:SECONDS       |
| HOUR_MINUTE    | HOURS:MINUTES               |
| DAY_SECOND     | DAYS HOURS:MINUTES:SECONDS  |
| DAY_MINUTE     | DAYS HOURS:MINUTES          |
| DAY_HOUR       | DAYS HOURS                  |
| YEAR_MONTH     | YEARS-MONTHS                |

```
SELECT DATE_ADD('1447671624', '1', 'SECOND')
-> '2015-11-16 11:00:25'
```

---
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



<br /><br />
Aggregate Functions
-------------------

---
### sum
Returns the sum of all values in the result set.

  sum(expr)


---
### count
Returns the number of values in the result set

  count(expr)


---
### mean
Returns the mean/average of values in the result set

    mean(expr)


---
### min
Returns the min of values in the result set

    min(expr)


---
### max
Returns the max of values in the result set

    max(expr)


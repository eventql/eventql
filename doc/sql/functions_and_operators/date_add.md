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


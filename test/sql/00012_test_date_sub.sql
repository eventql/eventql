-- ./sql_testdata/testtbl.cst
select
    date_sub(from_timestamp(1486913087), "10", "SECOND"),
    date_sub(from_timestamp(1486913087), "10:10", "MINUTE_SECOND"),
    date_sub(from_timestamp(1486913087), "01:10", "HOUR_MINUTE"),
    date_sub(from_timestamp(1486913087), "01:10:10", "HOUR_SECOND"),
    date_sub(from_timestamp(1486913087), "2 12:30:10", "DAY_SECOND"),
    date_sub(from_timestamp(1486913087), "2 12:30", "DAY_MINUTE"),
    date_sub(from_timestamp(1486913087), "2 12", "DAY_HOUR"),
    date_sub(from_timestamp(1486913087), "1-2", "YEAR_MONTH");


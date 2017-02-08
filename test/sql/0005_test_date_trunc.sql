-- ./sql_testdata/testtbl.cst
select
    date_trunc("msec", 1486553343542000),
    date_trunc("seconds", 1486553343542000),
    date_trunc("10minutes", 1486553343542000),
    date_trunc("5h", 1486553343542000),
    date_trunc("1d", 1486553343542000),
    date_trunc("1d", from_timestamp(1486553343)),
    date_trunc("1d", to_timestamp(1486553343542000));

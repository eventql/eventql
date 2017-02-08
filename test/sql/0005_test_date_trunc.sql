-- ./sql_testdata/testtbl.cst
select
    date_trunc("msec", from_timestamp(1486553343)),
    date_trunc("seconds", from_timestamp(1486553343)),
    date_trunc("10minutes", from_timestamp(1486553343)),
    date_trunc("5h", from_timestamp(1486553343)),
    date_trunc("1d", from_timestamp(1486553343));

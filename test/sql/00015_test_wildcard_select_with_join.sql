-- IMPORT customers FROM ./test/sql_testdata/testtbl2.csv
select * from customers c1 JOIN customers c2 ON c1.customerid = c2.customerid;

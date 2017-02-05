-- ./sql_testdata/testtbl.cst
DRAW LINECHART AXIS BOTTOM AXIS LEFT;

SELECT 'data' AS series, time AS x, count(1) AS y FROM testtable GROUP BY time ORDER BY time DESC limit 5;


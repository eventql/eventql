IMPORT TABLE cm_model
   FROM 'csv:conservative.csv?headers=true';

DRAW LINECHART WITH
    ORIENTATION VERTICAL
    SUBTITLE "cm conservative rev/net income/month w/ 4k burn rate, 10% business model, 50k funding after expenses"
    YDOMAIN -5000, 1000000
    AXIS BOTTOM TITLE "time (months)"
    AXIS LEFT
    GRID HORIZONTAL
    LEGEND TOP RIGHT INSIDE;

SELECT "net income" as series, month AS x, income AS y FROM cm_model ORDER BY month DESC;
SELECT "revenue" as series, month AS x, revenue AS y FROM cm_model ORDER BY month DESC;
SELECT "cumulative" as series, month AS x, cumulative AS y FROM cm_model ORDER BY month DESC;

DRAW LINECHART WITH
   XDOMAIN 0, 5 LOGARITHMIC
   YDOMAIN 0, 10000 LOGARITHMIC
   AXIS BOTTOM
   AXIS LEFT;

SELECT "log example" as series, x AS x, y AS y, "circle" as pointstyle
   FROM example_data;

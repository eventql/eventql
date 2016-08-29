DRAW BARCHART WITH
   ORIENTATION VERTICAL
   LEGEND TOP LEFT INSIDE
   AXIS BOTTOM;

SELECT city AS series, month AS x, temperature AS y
   FROM city_temperatures;

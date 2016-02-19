
var longstring = "FnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnordFnord";

Z1.processStream({
  table: "derivedts",
  from: 1445959789000000,
  until: 1446046160000000,
  calculate_fn: function(timerange_begin, timerange_limit) {

    var mapped = Z1.mapTable({
      table: "testtbl",
      map_fn: function(row) {
        console.log("mapmapmap");
        var tuples = [];
        for (var i = 0; i < 100; ++i) {
          tuples.push([row.segment1, { v: row.value, s: longstring }]);
        }
        return tuples;
      }
    });

    var reduced = Z1.reduce({
      sources: [mapped],
      shards: 4,
      reduce_fn: function(key, values) {
        var sum = 0;

        while (values.hasNext()) {
          sum += JSON.parse(values.next()).v;
        }

        return [[key, { dimension: key, value: sum, time: 1446045494000000 }]];
      }
    });

    return [reduced];
  }
});


function map_fn(row) {
  console.log("mapmapmap");
  return [[row.segment1, row.value]];
}

function reduce_fn(key, values) {
  var sum = 0;

  z1_log("reduce: " + key);
  while (values.hasNext()) {
    sum += parseInt(values.next(), 10);
  }

  return [[key, { dimension: key, value: sum, time: 1446045494000000 }]];
}

var mapped = Z1.mapTable({
  table: "testtbl",
  map_fn: map_fn
});

var reduced = Z1.reduce({
  sources: [mapped],
  shards: 1,
  reduce_fn: reduce_fn
});

Z1.saveToTablePartition({
  table: "derivedts",
  partition: "c573fde7aa8049d9cea101d93e5843893866af62",
  sources: [reduced]
});

Z1.downloadResults([reduced])

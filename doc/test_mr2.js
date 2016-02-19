var str = "MÄßstab";
console.log(str, str.toLowerCase());

var kNow = (new Date()).getTime() * 1000;                                               
var kMicrosPerDay = 24 * 60 * 60 * 1000000;      

var mapped = Z1.mapTable({
  table: "myts",
  from: kNow - kMicrosPerDay,
  until: kNow,
  map_fn: function(r) {
    console.log(r.value, r.value.toLowerCase(), 3);
    return [[r.value, r.value.toLowerCase()], [r.value.toLowerCase(), r.value]];
  }
});

Z1.downloadResults([mapped])

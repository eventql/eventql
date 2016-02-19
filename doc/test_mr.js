
var kPaymentMethods = {
  "1": "payment_method_banktransfer",
  "4": "payment_method_cash",
  "2": "payment_method_cash_on_delivery",
  "6": "payment_method_coupon",
  "3": "payment_method_paypal",
  "31": "payment_method_paypal",
  "10": "payment_method_wirecard_cc",
  "26": "payment_method_wirecard_eps",
  "22": "payment_method_wirecard_ideal",
  "18": "payment_method_wirecard_maestro",
  "30": "payment_method_wirecard_p24",
  "14": "payment_method_wirecard_sofort",
  "blah": function(x) { return 123; }
};

var kSomeConstant = 23;

function multiply(x) {
  return x * 2;
};

Z1.broadcast("kSomeConstant", "kPaymentMethods");

var jobs = [];
var i;

for (i = 0; i < 10; ++i) {
  Z1.broadcast("i");

  var mapped = Z1.mapTable({
    table: "testtbl",
    map_fn: function(row) {
      return [[row.segment1, multiply(row.value * kSomeConstant * kPaymentMethods.blah())]];
    }
  });


  var reduced = Z1.reduce({
    sources: [mapped],
    shards: 1,
    reduce_fn: function(key, values) {
      //console.log("reduce", key, values);
      var sum = 0;

      while (values.hasNext()) {
        sum += parseInt(values.next(), 10);
      }

      return [[key, sum]];
    }
  });

  jobs.push(reduced);
}

var merged = Z1.reduce({
  sources: jobs,
  shards: 1,
  reduce_fn: function(key, values) {
    //console.log("reduce", key, values);
    var tuples = [];

    while (values.hasNext()) {
      tuples.push([key, values.next()]);
    }

    return tuples;
  }
});

Z1.writeToOutput("blaaaaaah");

Z1.downloadResults([merged], function(key, value) {
  return JSON.stringify([key, value]);
});

var ZBaseSellerMetrics = (function() {
  var add = function(a, b) {
    return parseFloat(a) + parseFloat(b);
  };

  var sum = function(values) {
    return values.reduce(add, 0);
  };

  var mean = function(values) {
    return (sum(values) / values.length);
  };

  var numberFormat = function(num, precision) {
    var parts = num.toString().split(".");

    //integer format with , every n digits
    var int_parts = parts[0].split("").reverse();
    var integer = [];
    var n = 3;

    for (var i = 0; i < int_parts.length; i += n) {
      integer.push(int_parts.slice(i, i + n).reverse().join(""));
    }
    integer = integer.reverse().join(",");

    if (precision == 0) {
      return integer;
    }

    //decimal part with exactly n digits, where n = precision
    if (parts.length == 1) {
      parts.push("");
    }

    var dec = [];
    for (var i = 0; i < precision; i++) {
      if (i < parts[1].length) {
        dec.push(parts[1][i]);
      } else {
        dec.push("0");
      }
    }

    return integer + "." + dec.join("");
  };

  var printEurcent = function(amount) {
    if (isNaN(amount)) {
      return "-";
    }

    return numberFormat(amount / 100, 3) + "&euro;";
  };

  var printTimestamp = function(ts) {
    return DateUtil.printDate(Math.round(ts / 1000));
  };

  var printNumber = function(precision) {
    return function(num) {
      if (isNaN(num)) {
        return "-";
      }

      return numberFormat(num, precision);
    }
  };

  var print = function(value) {
    return value;
  }

  var printAsPercent = function(num) {
    if (isNaN(num)) {
      return "-";
    }


    return num;
  };

  return {
    shop_id: {print: print},
    time: {print: printTimestamp},
    gmv_eurcent: {aggr: sum, print: printEurcent},
    gmv_per_transaction_eurcent: {aggr: mean, print: printEurcent},
    num_purchases: {aggr: sum, print: printNumber(0)},
    num_refunds: {aggr: sum, print: printNumber(0)},
    refund_rate: {aggr: sum, print: printNumber(2)},
    refunded_gmv_eurcent: {aggr: sum, print: printEurcent},
    listview_views_ads: {aggr: sum, print: printNumber(0)},
    listview_views_search_page: {aggr: sum, print: printNumber(0)},
    listview_views_catalog_page: {aggr: sum, print: printNumber(0)},
    listview_views_recos: {aggr: sum, print: printNumber(0)},
    listview_views_shop_page: {aggr: sum, print: printNumber(0)},
    listview_clicks_ads: {aggr: sum, print: printNumber(0)},
    listview_clicks_search_page: {aggr: sum, print: printNumber(0)},
    listview_clicks_catalog_page: {aggr: sum, print: printNumber(0)},
    listview_clicks_recos: {aggr: sum, print: printNumber(0)},
    listview_clicks_shop_page: {aggr: sum, print: printNumber(0)},
    listview_ctr_ads: {aggr: mean, print: printAsPercent},
    listview_ctr_search_page: {aggr: mean, print: printAsPercent},
    listview_ctr_catalog_page: {aggr: mean, print: printasPercent},
    listview_ctr_recos: {aggr: mean, print: printAsPercent},
    listview_ctr_shop_page: {aggr: mean, print: printAsPercent},
    num_active_products: {aggr: sum, print: printNumber(0)},
    num_listed_products: {aggr: sum, print: printNumber(0)},
    shop_page_views: {aggr: sum, print: printNumber(0)},
    product_page_views: {aggr: sum, print: printNumber(0)},
    total_listviews: {aggr: sum, print: printNumber(0)}
  }
})();

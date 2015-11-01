var ZBaseSellerMetrics = (function() {
  var add = function(a, b) {
    return parseFloat(a), parseFloat(b);
  };

  var sum = function(values) {
    return values.reduce(add, 0);
  };

  var mean = function(values) {
    return (sum(values) / values.length);
  };

  var round = function(num, precision) {
    return (Math.round(num * Math.pow(10, precision)) / Math.pow(10, precision));
  };

  var printEurcent = function(amount) {
    if (isNaN(amount)) {
      return "-";
    }

    return round(amount / 100, 3) + "&euro;";
  };

  var printTimestamp = function(ts) {
    return DateUtil.printDate(Math.round(ts / 1000));
  };

  var print = function(precision) {
    return function(value) {
      if (isNaN(value)) {
        return "-";
      }

      return round(value, precision);
    }
  };


  return {
    time: {print: printTimestamp},
    gmv_eurcent: {aggr: sum, print: printEurcent},
    gmv_per_transaction_eurcent: {aggr: mean, print: printEurcent},
    num_purchases: {aggr: sum, print: print(0)},
    num_refunds: {aggr: sum, print: print(0)},
    refund_rate: {aggr: sum, print: print(2)},
    refunded_gmv_eurcent: {aggr: sum, print: printEurcent},
    listview_views_ads: {aggr: sum, print: print(0)},
    listview_views_search_page: {aggr: sum, print: print(0)},
    listview_views_catalog_page: {aggr: sum, print: print(0)},
    listview_views_recos: {aggr: sum, print: print(0)},
    listview_views_shop_page: {aggr: sum, print: print(0)},
    listview_clicks_ads: {aggr: sum, print: print(0)},
    listview_clicks_search_page: {aggr: sum, print: print(0)},
    listview_clicks_catalog_page: {aggr: sum, print: print(0)},
    listview_clicks_recos: {aggr: sum, print: print(0)},
    listview_clicks_shop_page: {aggr: sum, print: print(0)},
    listview_ctr_ads: {aggr: mean, print: print(4)},
    listview_ctr_search_page: {aggr: mean, print: print(4)},
    listview_ctr_catalog_page: {aggr: mean, print: print(4)},
    listview_ctr_recos: {aggr: mean, print: print(4)},
    listview_ctr_shop_page: {aggr: mean, print: print(4)},
    num_active_products: {aggr: sum, print: print(0)},
    num_listed_products: {aggr: sum, print: print(0)},
    shop_page_views: {aggr: sum, print: print(0)},
    product_page_views: {aggr: sum, print: print(0)},
    total_listviews: {aggr: sum, print: print(0)}
  }
})();

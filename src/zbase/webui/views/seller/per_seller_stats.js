ZBase.registerView((function() {
  var path_prefix = "/a/seller/";
  var seller_id;

  var load = function(path) {
    seller_id = path.substr(path_prefix.length);
    //REMOVE ME
    var seller_name = "Meko";
    //REMOVEME END

    var page = $.getTemplate(
        "views/seller",
        "per_seller_stats_main_tpl");

    $(".zbase_stats_title .shop_name", page).innerHTML =
        seller_id + " (" + seller_name + ")";
    $("z-daterangepicker", page).addEventListener("select", paramChanged, false);

    $.handleLinks(page);
    $.replaceViewport(page);

    setParamsFromAndUntil(
        UrlUtil.getParamValue(path, "from"),
        UrlUtil.getParamValue(path, "until"));

    //REMOVE ME
    var data = {
      aggregates: {
        unique_purchases: 7,
        refunded_purchases: 0,
        gmv_eurcent: 192,
        num_active_products: 128,
        num_listed_products: 194,
        refund_rate: 0.0,
        refunded_gmv_eurcent: 0,
        gmv_per_transaction_eurcent: 40
      },
      timeseries: [
        {
          unique_purchases: 2,
          refunded_purchases: 0,
          gmv_eurcent: 50,
          num_active_products: 128,
          num_listed_products: 194,
          refund_rate: 0.0,
          refunded_gmv_eurcent: 0,
          gmv_per_transaction_eurcent: 27
        },
        {
          unique_purchases: 0,
          refunded_purchases: 0,
          gmv_eurcent: 0,
          num_active_products: 128,
          num_listed_products: 194,
          refund_rate: 0.0,
          refunded_gmv_eurcent: 0,
          gmv_per_transaction_eurcent: 0
        },
        {
          unique_purchases: 1,
          refunded_purchases: 0,
          gmv_eurcent: 50,
          num_active_products: 128,
          num_listed_products: 194,
          refund_rate: 0.0,
          refunded_gmv_eurcent: 0,
          gmv_per_transaction_eurcent: 50
        },
        {
          unique_purchases: 3,
          refunded_purchases: 0,
          gmv_eurcent: 82,
          num_active_products: 128,
          num_listed_products: 194,
          refund_rate: 0.0,
          refunded_gmv_eurcent: 0,
          gmv_per_transaction_eurcent: 32
        },
        {
          unique_purchases: 1,
          refunded_purchases: 0,
          gmv_eurcent: 10,
          num_active_products: 128,
          num_listed_products: 194,
          refund_rate: 0.0,
          refunded_gmv_eurcent: 0,
          gmv_per_transaction_eurcent: 10
        }
      ]
    };
    //REMOVEME END

    render(data);
  };

  var destroy = function() {
    window.removeEventListener("resize", resizeSparklines);
  };

  var render = function(data) {
    var sparkline_values = {};
    data.timeseries.forEach(function(serie) {
      for (metric in serie) {
        if (!sparkline_values[metric]) {
          sparkline_values[metric] = [];
        }
        sparkline_values[metric].push(serie[metric]);
      }
    });

    for (var metric in data.aggregates) {
      var pane = $(".zbase_seller_stats ." + metric);
      $(".num", pane).innerHTML = data.aggregates[metric];
      $("z-sparkline", pane).setAttribute(
          "data-sparkline",
          sparkline_values[metric].join(","));

      //remove if
      if ($(".zbase_seller_stats z-tooltip." + metric)) {
        $(".zbase_seller_stats z-tooltip." + metric).init($("i.help", pane));
      }
    }
  };

  var setParamsFromAndUntil = function(from, until) {
    if (!until) {
      until = DateUtil.getStartOfDay(Date.now());
    }

    if (!from) {
      from = until - DateUtil.millisPerDay * 30;
    }

    $(".zbase_seller_stats z-daterangepicker").setValue(
        from, until);
  };

  var getUrl = function() {
    var params = $(".zbase_seller_stats z-daterangepicker").getValue();
    return path_prefix + seller_id + "?" + $.buildQueryString(params);
  };

  var paramChanged = function(e) {
    $.navigateTo(getUrl());
  };

  var resizeSparklines = function() {
    var sparklines = document.querySelectorAll(".zbase_seller_stats z-sparkline");
    for (var i = 0; i < sparklines.length; i++) {
      sparklines[i].render();
    }
  };

  window.addEventListener("resize", resizeSparklines);

  return {
    name: "per_seller_stats",
    loadView: function(params) {load(params.path)},
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

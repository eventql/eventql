ZBase.registerView((function() {
  var path_prefix = "/a/seller/";
  var query_mgr;
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

    query_mgr = EventSourceHandler();

    var query_str =
      "select FROM_TIMESTAMP(time / 1000), gmv_eurcent, gmv_per_transaction_eurcent, " +
      "num_purchases, num_refunds, refund_rate, refunded_gmv_eurcent, listview_views_ads, " +
      "listview_ctr_ads, listview_clicks_ads, listview_views_recos, listview_ctr_recos, " +
      "listview_clicks_recos, listview_views_search_page, listview_ctr_search_page, " +
      "listview_clicks_search_page, listview_views_catalog_page, listview_ctr_catalog_page, " +
      "listview_clicks_catalog_page, shop_page_views, product_page_views, listview_views_shop_page, " +
      "listview_ctr_shop_page, listview_clicks_shop_page, num_active_products, " +
      "num_listed_products from shop_stats.last30d where shop_id = " +
      seller_id + "order by time asc limit 100;";


    var query = query_mgr.get(
        "sql_query",
        "/api/v1/sql_stream?query=" + encodeURIComponent(query_str));

    query.addEventListener("result", function(e) {
      query_mgr.close("sql_query");
      var data = JSON.parse(e.data);
      renderView(path, data.results[0]);
    }, false);

    query.addEventListener("error", function(e) {
      query_mgr.close("sql_query");
      console.log("error", e);
    }, false);

    query.addEventListener("status", function(e) {
      renderQueryProgress(JSON.parse(e.data));
    }, false);

    setParamsFromAndUntil(
        UrlUtil.getParamValue(path, "from"),
        UrlUtil.getParamValue(path, "until"));

  };

  var destroy = function() {
    window.removeEventListener("resize", resizeSparklines);
    query_mgr.closeAll();
  };

  var renderQueryProgress = function(progress) {
    QueryProgressWidget.render(
        $(".zbase_seller_overview"),
        progress);
  };

  var renderView = function(path, result) {
    var view = UrlUtil.getParamValue(path, "view");
    if (!view || view == "table") {
      PerSellerTableOverview.render($(".zbase_seller_overview"), result);
    } else {
      PerSellerSparklineOverview.render($(".zbase_seller_overview"), result);
    }
  };

  var renderSparklines = function(data) {
    console.log(data);
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

  var renderTable = function(result) {
    console.log(result);
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

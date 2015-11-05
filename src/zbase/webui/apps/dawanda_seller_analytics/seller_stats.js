ZBase.registerView((function() {
  var path_prefix = "/a/apps/dawanda_seller_analytics/";
  var result = null;
  var query_mgr;
  var shop_id;

  var load = function(path) {
    query_mgr = EventSourceHandler();
    shop_id = UrlUtil.getPath(path).substr(path_prefix.length);

    var page = $.getTemplate(
        "views/seller",
        "per_seller_stats_main_tpl");

    var bc = $("zbase-breadcrumbs-section a.shop_id", page);
    bc.href = path_prefix + shop_id;
    bc.innerHTML = "Seller " + shop_id;
    $(".pagetitle .shop_id", page).innerHTML = shop_id;

    $.onClick($("button.view.sparkline", page), onViewButtonClick);
    $.onClick($("button.view.table", page), onViewButtonClick);
    $.onClick($("button.view.csv_download", page), onViewButtonClick);
    $("z-dropdown.metrics", page).addEventListener("change", onMetricsParamChanged);

    //REMOVEME (replace with paramChanged)
    var date_picker = $("z-daterangepicker", page);
    $("z-daterangepicker-field", page).onclick = function(e) {
      date_picker.removeAttribute("data-active");
      alert("Not yet implemented");
    };
    //REMOVEME END

    loadShopName($(".pagetitle .shop_name", page));

    $.handleLinks(page);
    $.replaceViewport(page);

    var query_str =
      "select" +
          " time," +
          " num_active_products," +
          " num_listed_products," +
          " num_purchases," +
          " num_refunds," +
          " refund_rate," +
          " gmv_eurcent," +
          " refunded_gmv_eurcent," +
          " gmv_per_transaction_eurcent," +
          " shop_page_views," +
          " product_page_views," +
          " listview_views_search_page + listview_views_shop_page + listview_views_catalog_page + listview_views_ads + listview_views_recos as total_listviews," +
          " listview_views_search_page," +
          " listview_clicks_search_page," +
          " listview_ctr_search_page," +
          " listview_views_catalog_page," +
          " listview_clicks_catalog_page," +
          " listview_ctr_catalog_page," +
          " listview_views_shop_page," +
          " listview_clicks_shop_page," +
          " listview_ctr_shop_page," +
          " listview_views_ads," +
          " listview_clicks_ads," +
          " listview_ctr_ads," +
          " listview_views_recos," +
          " listview_clicks_recos," +
          " listview_ctr_recos" +
      " from shop_stats.last30d where shop_id = " + shop_id // FIXME escaping
      " order by time asc;";

    setParamsFromAndUntil(
        UrlUtil.getParamValue(path, "from"),
        UrlUtil.getParamValue(path, "until"));

    setParamView(UrlUtil.getParamValue(path, "view"));
    setParamMetrics(UrlUtil.getParamValue(path, "metrics"));

    var query = query_mgr.get(
        "sql_query",
        "/api/v1/sql_stream?query=" + encodeURIComponent(query_str));

    query.addEventListener("result", function(e) {
      query_mgr.close("sql_query");
      var data = JSON.parse(e.data).results[0];

      if (data.rows.length == 0) {
        //renderNoDataReturned();
        //return;
      }

      var values = [];
      for (var i = 0; i < data.columns.length; i++) {
        values.push([]);
      };

      for (var i = 0; i < data.rows.length; i++) {
        for (var j = 0; j < data.rows[i].length; j++) {
          values[j].push(data.rows[i][j]);
        }
      }

      result = {
        timeseries: {},
      };
      for (var i = 0; i < data.columns.length; i++) {
        result.timeseries[data.columns[i]] = values[i];
      }

      result.aggregates = aggregate(result.timeseries);

      $(".zbase_seller_stats .controls").classList.remove("hidden");
      /*setParamsFromAndUntil(
          Math.round(data.rows[0][0] / 1000),
          Math.round(data.rows[data.rows.length - 1][0] / 1000));*/
      renderView(path);
    }, false);

    query.addEventListener("error", function(e) {
      query_mgr.close("sql_query");
      console.log("error", e);
    }, false);

    query.addEventListener("status", function(e) {
      renderQueryProgress(JSON.parse(e.data));
    }, false);
  };

  var destroy = function() {
    window.removeEventListener("resize", resizeSparklines);
    query_mgr.closeAll();
  };

  var loadShopName = function(elem) {
    var query_str = "SELECT `title` FROM db.shops WHERE id = " +
      $.escapeHTML(shop_id);

    var query = query_mgr.get(
      "shop_name",
      "/api/v1/sql_stream?query=" + encodeURIComponent(query_str));

    query.addEventListener("result", function(e) {
      query_mgr.close("shop_name");
      var result = JSON.parse(e.data).results[0];
      elem.innerHTML = "(" + result.rows[0] + ")";
    });

    query.addEventListener("error", function(e) {
      query_mgr.close("shop_name");
    });
  };

  var renderQueryProgress = function(progress) {
    QueryProgressWidget.render(
        $(".zbase_seller_overview"),
        progress);
  };

  var renderView = function(path) {
    if (result == null) {
      $.fatalError();
    }

    var metrics = getMetrics();
    var view = getParamView();

    switch (view) {
      case "table":
        SellerTableOverview.render(
          $(".zbase_seller_overview"),
          result,
          metrics);
        break;

      case "sparkline":
        SellerSparklineOverview.render(
          $(".zbase_seller_overview"),
          result,
          metrics);
        break;

      case "csv_download":
        SellerCSVDownload.render($(".zbase_seller_overview"));
        break;
    }

    updateControlsVisibility(view);
  };

  var renderNoDataReturned = function() {
    $.replaceContent(
      $(".zbase_seller_stats .zbase_content_pane"),
      $.getTemplate(
          "views/seller",
          "per_seller_stats_no_data_message_tpl"));
  };

  var aggregate = function(timeseries) {
    var aggregates = {};

    for (var metric in timeseries) {
      if (metric == "time" || metric == "shop_id") {
        continue;
      }

      aggregates[metric] = ZBaseSellerMetrics[metric].print(
          ZBaseSellerMetrics[metric].aggr(timeseries[metric]));
    }

    return aggregates;
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

  var setParamView = function(view) {
    if (view == null) {
      view = "sparkline"
    }

    $(".zbase_seller_stats button.view." + view)
        .setAttribute("data-state", "active");
  };

  var getParamView = function() {
    return $(".zbase_seller_stats button.view[data-state='active']").getAttribute(
        "data-value");
  };

  var setParamMetrics = function(metrics) {
    if (metrics) {
      $(".zbase_seller_stats z-dropdown.metrics").setValue(
          decodeURIComponent(metrics).split(","));
    }
  };

  var getMetrics = function() {
    var metrics = {time: true};
    $(".zbase_seller_stats z-dropdown.metrics")
        .getValue()
        .split(",")
        .forEach(function(metric) {
          metrics[metric] = true;
        });

    return metrics;
  };

  var getUrl = function() {
    //var params = $(".zbase_seller_stats z-daterangepicker").getValue();
    var params = {};
    params.view = $(".zbase_seller_stats button.view[data-state='active']")
        .getAttribute("data-value");
    params.metrics = encodeURIComponent(
        $(".zbase_seller_stats z-dropdown.metrics").getValue());
    return path_prefix + shop_id + "?" + $.buildQueryString(params);
  };

  var paramChanged = function() {
    $.navigateTo(getUrl());
  };

  var onViewButtonClick = function() {
    $(".zbase_seller_stats button.view[data-state='active']").removeAttribute(
        "data-state");

    this.setAttribute("data-state", "active");

    renderView();

    $.pushHistoryState(getUrl());
  };

  var onMetricsParamChanged = function() {
    renderView();

    $.pushHistoryState(getUrl());
  };

  var resizeSparklines = function() {
    var sparklines = document.querySelectorAll(".zbase_seller_stats z-sparkline");
    for (var i = 0; i < sparklines.length; i++) {
      sparklines[i].render();
    }
  };

  var updateControlsVisibility = function(view) {
    var metrics_control = $(".zbase_seller_stats .control.metrics");
    var datepicker_control = $(".zbase_seller_stats .control.timerange");

    switch (view) {
      case "table":
        metrics_control.classList.remove("hidden");
        datepicker_control.classList.remove("hidden");
        return;

      case "sparkline":
        metrics_control.classList.add("hidden");
        datepicker_control.classList.remove("hidden");
        return;

      case "csv_download":
        metrics_control.classList.add("hidden");
        datepicker_control.classList.add("hidden");
        return;
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

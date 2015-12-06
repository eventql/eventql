ZBase.registerView((function() {
  var path_prefix = "/a/apps/dawanda_seller_analytics/";
  var shop_id;
  var query_mgr;

  var load = function(path) {
    query_mgr = EventSourceHandler();
    shop_id = UrlUtil.getPath(path).substr(path_prefix.length);

    var layout = perSellerLayout(query_mgr, path_prefix, shop_id);
    layout.render();

    var query_str =
      "select" +
          " time," +
          " gmv_eurcent," +
          " num_purchases," +
          " num_refunds," +
          " refunded_gmv_eurcent," +
          " gmv_per_transaction_eurcent," +
          " refund_rate" +
          " from shop_stats.last30d where shop_id = " + $.escapeHTML(shop_id)
          " order by time asc;";

    var query = query_mgr.get(
        "sql_query",
        "/api/v1/sql?format=json_sse&query=" + encodeURIComponent(query_str));

    query.addEventListener("result", function(e) {
      query_mgr.close("sql_query");
      var data = JSON.parse(e.data).results[0];

      if (data.rows.length == 0) {
        layout.renderNoDataReturned();
        return;
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

      var result = {
        timeseries: {},
      };
      for (var i = 0; i < data.columns.length; i++) {
        result.timeseries[data.columns[i]] = values[i];
      }

      result.aggregates = aggregate(result.timeseries);

      renderResult(result);
    }, false);


    query.addEventListener("error", function(e) {
      query_mgr.close("sql_query");
      console.log("error", e);
    }, false);

    query.addEventListener("status", function(e) {
      layout.renderQueryProgress(JSON.parse(e.data));
    }, false);
  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }
  };

  var renderResult = function(result) {
    var tpl = $.getTemplate(
        "apps/dawanda_seller_analytics",
        "per_seller_revenue_main_tpl");

    $.replaceContent($(".zbase_seller_stats.per_seller .result_pane"), tpl);

    var start = DateUtil.printDate(Math.round(
        result.timeseries.time[0] / 1000));
    var end = DateUtil.printDate(Math.round(
        result.timeseries.time[result.timeseries.time.length - 1] / 1000));

    var container = $(".zbase_seller_stats.per_seller .revenue");

    //render panes multiple lines
    var stacked_panes = container.querySelectorAll(".metric_pane.stacked");
    for (var i = 0; i < stacked_panes.length; i++) {
      var pane = stacked_panes[i];
      var metrics = pane.getAttribute("data-metric").split(",");
      var colors = ["#9cc9f3", "#f4bbca"];
      var metric_values = [];

      for (var j = 0; j < metrics.length; j++) {
        metric_values.push({
            name: metrics[j],
            values: result.timeseries[metrics[j]].map(function(v) {
              return parseFloat(v);
            }),
            color: colors[j]
          });
      }

      var z_chart = $("z-chart", pane);
      z_chart.renderLines({
          x: result.timeseries.time,
          y: metric_values});

      z_chart.formatX = function(value) {
        return ZBaseSellerMetrics.time.print(value);
      };

      z_chart.formatY = function(value, metric) {
        return ZBaseSellerMetrics[metric].print(value);
      };

      for (var j = 0; j < metrics.length; j++) {
        $(".num ." + metrics[j], pane).innerHTML = result.aggregates[metrics[j]];
      }

      $(".start", pane).innerHTML = start;
      $(".end", pane).innerHTML = end;
    }

    //render panes with line chart
    var line_panes = container.querySelectorAll(".metric_pane.line");
    for (var i = 0; i < line_panes.length; i++) {
      var pane = line_panes[i];
      var metric = pane.getAttribute("data-metric");

      var z_chart = $("z-chart", pane);
      z_chart.render(
          result.timeseries.time,
          result.timeseries[metric]
            .map(function(v) {
              return parseFloat(v);
            }));

      z_chart.formatX = function(value) {
        return ZBaseSellerMetrics.time.print(value);
      };

      z_chart.formatY = formatY(metric);

      $(".num", pane).innerHTML = result.aggregates[metric];
      $(".start", pane).innerHTML = start;
      $(".end", pane).innerHTML = end;
    }
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

  var formatY = function(metric) {
    return function(value) {
      return ZBaseSellerMetrics[metric].print(value);
    }
  };

  return {
    name: "per_seller_revenue",
    loadView: function(params) {load(params.path)},
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

ZBase.registerView((function() {
  var path_prefix = "/a/apps/dawanda_seller_analytics/inventory/";
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
          " gmv_per_transaction_eurcent," +
          " num_purchases," +
          " product_page_views," +
          " listview_views_search_page + listview_views_shop_page + listview_views_catalog_page + listview_views_ads + listview_views_recos as total_listviews," +
          " num_listed_products," +
          " buy_to_detail_rate" +
      " from shop_stats.last30d where shop_id = " + $.escapeHTML(shop_id)
      " order by time asc;";

    var query = query_mgr.get(
        "sql_query",
        "/api/v1/sql?format=json_sse&query=" + encodeURIComponent(query_str));

    query.addEventListener("result", function(e) {
      query_mgr.close("sql_query");
      var data = JSON.parse(e.data).results[0];

      if (data.rows.length == 0) {
        layout.renderNoData();
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

      renderResult(result)
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
        "per_seller_inventory_main_tpl");

    //render table
    var tbody = $("tbody", tpl);
    var tr_tpl = $.getTemplate(
        "apps/dawanda_seller_analytics",
        "per_seller_inventory_row_tpl");

    var aggr_tr = tr_tpl.cloneNode(true);
    for (var metric in result.aggregates) {
      //init help bubble
      var th = $("th." + metric, tpl);
      if (th) {
        $("z-tooltip." + metric, tpl).init($(".help", th));
      }

      //render aggregate field
      var td = $("td." + metric, aggr_tr);
      if (td) {
        td.innerHTML = result.aggregates[metric];
      }
    }
    $(".time", aggr_tr).innerHTML = "Total";
    tbody.appendChild(aggr_tr);

    for (var i = 0; i < result.timeseries.time.length; i++) {
      var tr = tr_tpl.cloneNode(true);

      for (var metric in result.timeseries) {
        var td = $("td." + metric, tr);
        if (td) {
          td.innerHTML = ZBaseSellerMetrics[metric].print(
            result.timeseries[metric][i]);
        }
      }

      tbody.appendChild(tr);
    }

    $.replaceContent($(".zbase_seller_stats.per_seller .result_pane"), tpl);

    //render metric panes
    var start = DateUtil.printDate(Math.round(
        result.timeseries.time[0] / 1000));
    var end = DateUtil.printDate(Math.round(
        result.timeseries.time[result.timeseries.time.length - 1] / 1000));

    for (var metric in result.timeseries) {
      var pane = $(
          ".zbase_seller_stats.per_seller .inventory .metric_pane." + metric);

      if (pane) {
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
    name: "per_seller_inventory",
    loadView: function(params) {load(params.path)},
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

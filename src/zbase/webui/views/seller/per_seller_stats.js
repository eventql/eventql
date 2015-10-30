ZBase.registerView((function() {
  var path_prefix = "/a/seller/";
  var result = null;
  var query_mgr;
  var seller_id;

  var load = function(path) {
    seller_id = UrlUtil.getPath(path).substr(path_prefix.length);
    //REMOVE ME
    var seller_name = "Meko";
    //REMOVEME END

    var page = $.getTemplate(
        "views/seller",
        "per_seller_stats_main_tpl");

    $(".zbase_stats_title .shop_name", page).innerHTML =
        seller_id + " (" + seller_name + ")";
    $("z-daterangepicker", page).addEventListener("select", paramChanged, false);

    $.onClick($("button.view.sparkline", page), onViewButtonClick);
    $.onClick($("button.view.table", page), onViewButtonClick);


    $.handleLinks(page);
    $.replaceViewport(page);

    query_mgr = EventSourceHandler();

    var query_str =
      "select * from shop_stats.last30d where shop_id = " +
      seller_id + "order by time asc limit 100;";


    setParamsFromAndUntil(
        UrlUtil.getParamValue(path, "from"),
        UrlUtil.getParamValue(path, "until"));

    setParamView(UrlUtil.getParamValue(path, "view"));

    var query = query_mgr.get(
        "sql_query",
        "/api/v1/sql_stream?query=" + encodeURIComponent(query_str));

    query.addEventListener("result", function(e) {
      query_mgr.close("sql_query");
      var data = JSON.parse(e.data).results[0];

      var values = [];
      for (var i = 0; i < data.rows[0].length; i++) {
        values.push([]);
      };

      for (var i = 0; i < data.rows.length; i++) {
        for (var j = 0; j < data.rows[i].length; j++) {
          values[j].push(data.rows[i][j]);
        }
      }

      result = {
        aggregates: {},
        timeseries: {},
      };
      for (var i = 0; i < data.columns.length; i++) {
        result.timeseries[data.columns[i]] = values[i];
      }

      aggregate(result.timeseries);

      //TODO get aggregates

      renderView();
    }, false);

    query.addEventListener("error", function(e) {
      query_mgr.close("sql_query");
      console.log("error", e);
    }, false);

    query.addEventListener("status", function(e) {
      renderQueryProgress(JSON.parse(e.data));
    }, false);
  };

  var aggregate = function(timeseries) {
    var aggregates = {};
    var add = function(a, b) {
      return parseFloat(a), parseFloat(b);
    };

    var sum = function(values) {
      return values.reduce(add, 0);
    };

    //arithmetic mean
    var mean = function(values) {
      return (sum(values) / values.length);
    }

    var toPrecision = function(num, precision) {
      return Math.round(num * Math.pow(10, precision)) / Math.pow(10, precision);
    };

    var aggr = function(values, fn, precision) {
      return toPrecision(fn(values), precision);
    }


    var metrics = {
      gmv_eurcent: {
        fn: sum,
        precision: 2
      },
      gmv_per_transaction_eurcent: {
        fn: mean,
        precision: 2
      }
    };

    for (var metric in timeseries) {
      if (metrics.hasOwnProperty(metric)) {
        aggregates[metric] = aggr(
          timeseries[metric],
          metrics[metric].fn,
          metrics[metric].precision);
      }
    }

    console.log(aggregates);
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

  var renderView = function() {
    if (result == null) {
      $.fatalError();
    }

    var view = getParamView();
    if (view == "table") {
      PerSellerTableOverview.render($(".zbase_seller_overview"), result);
    } else {
      PerSellerSparklineOverview.render($(".zbase_seller_overview"), result);
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

  var getUrl = function() {
    var params = $(".zbase_seller_stats z-daterangepicker").getValue();
    params.view = $(".zbase_seller_stats button.view[data-state='active']")
        .getAttribute("data-value");
    return path_prefix + seller_id + "?" + $.buildQueryString(params);
  };

  var paramChanged = function() {
    $.navigateTo(getUrl());
  };

  var onViewButtonClick = function() {
    $(".zbase_seller_stats button.view[data-state='active']").removeAttribute(
        "data-state");

    this.setAttribute("data-state", "active");
    renderView();
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

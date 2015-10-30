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
        "select * from shop_stats.last30d where shop_id = " +
        seller_id + "order by time asc limit 100;";


    var query = query_mgr.get(
        "sql_query",
        "/api/v1/sql_stream?query=" + encodeURIComponent(query_str));

    query.addEventListener("result", function(e) {
      query_mgr.close("sql_query");
      var data = JSON.parse(e.data);
      renderTable(data.results[0]);
    }, false);

    query.addEventListener("error", function(e) {
      query_mgr.close("sql_query");
      console.log("error", e);
    }, false);

    query.addEventListener("status", function(e) {
      //TODO render loader
    }, false);

    setParamsFromAndUntil(
        UrlUtil.getParamValue(path, "from"),
        UrlUtil.getParamValue(path, "until"));

  };

  var destroy = function() {
    window.removeEventListener("resize", resizeSparklines);
    query_mgr.closeAll();
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

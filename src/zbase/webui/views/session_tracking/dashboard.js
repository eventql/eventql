ZBase.registerView((function() {
  $.showLoader();
  var query_mgr;
  var chart;

  var load = function(path) {
    query_mgr = EventSourceHandler();

    var page = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_dashboard_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), path);

    $.handleLinks(page);
    $.replaceViewport(page);

    setParamMetric(UrlUtil.getParamValue(path, "metric"));
    setParamTimeWindow(UrlUtil.getParamValue(path, "time_window"));
    setParamsFromAndUntil(
        UrlUtil.getParamValue(path, "from"),
        UrlUtil.getParamValue(path, "until"));

    $(".zbase_session_tracking_dashboard z-dropdown.metric")
        .addEventListener("change", paramChanged);
    $(".zbase_session_tracking_dashboard z-dropdown.time_window")
        .addEventListener("change", paramChanged);

    //REMOVEME
    $(".zbase_session_tracking_dashboard z-daterangepicker")
        .toggleWidgetVisibility = function() {
            alert("Not yet implemented");
        };
    //REMOVEME END

    render();
    $.hideLoader();
  };


  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }

    if (chart) {
      chart.destroy();
    }
  };

  var render = function() {
    renderLoader();
    destroy();

    var query_string = buildQueryString();
    var query = query_mgr.get(
      "sql_query",
      "/api/v1/sql_stream?query=" + encodeURIComponent(query_string));

    query.addEventListener('result', function(e) {
      query_mgr.close("sql_query");
      var data = JSON.parse(e.data);
      if (!data.results[0].rows || data.results[0].rows.length == 0) {
        renderError();
      }
      renderChart(data.results);
      hideLoader();
    });

    query.addEventListener('error', function(e) {
      console.log(e);
      query_mgr.close("sql_query");
      $.fatalError("Server Error");
    });
  };

  var renderQueryProgress = function(progress) {
    $(".zbase_session_tracking_dashboard .query_progress").classList.remove("hidden");
    $(".zbase_session_tracking_dashboard .dashboard").classList.add("hidden");

    QueryProgressWidget.render(
        $(".zbase_session_tracking_dashboard .query_progress"),
        progress);
  };

  var renderChart = function(results) {
    var x_values = [];
    var y_values = [];

    results[0].rows.forEach(function(point) {
      x_values.push(parseInt(point[0], 10));
      y_values.push(point[1]);
    });

    var chart_config = {
      data: {
        x: x_values,
        y: [
          {
            name: getMetricTitle(),
            values: y_values,
            color: "#3498db"
          }
        ]
      },
      point: {
        show: false
      },
      type: 'area',
      format: '%Y-%m-%d'
    };
    if (getParamTimeWindow() == "3600") {
      chart_config.format += " %H:%M";
    }

    chart = ZBaseC3Chart(chart_config);

    var chart_container = $(".zbase_session_tracking_dashboard .chart_container");
    var padding = 32;
    chart.chartWidth(function() {
      return chart_container.offsetWidth - padding;
    });
    chart.renderTimeseries("dashboard_chart");
    chart.renderLegend($(".zbase_session_tracking_dashboard .chart_legend"));
  };

  var renderError = function() {
    var inner_chart = $(".zbase_session_tracking_dashboard .inner_chart");
    inner_chart.innerHTML = "<div class='error_msg'>No Data Returned</div>";
  };

  var renderLoader = function() {
    $(".zbase_session_tracking_dashboard .zbase_loader").classList.remove("hidden");
    $(".zbase_session_tracking_dashboard .inner_chart").classList.add("hidden");
  };

  var hideLoader = function() {
    $(".zbase_session_tracking_dashboard .zbase_loader").classList.add("hidden");
    $(".zbase_session_tracking_dashboard .inner_chart").classList.remove("hidden");
  };

  var buildQueryString = function() {
    var time_window = parseInt(getParamTimeWindow(), 10);
    switch (getParamMetric()) {
      case "num_sessions":
        return "select TRUNCATE(time / " + time_window * 1000000 + ") * " + time_window * 1000 +
        " as time, count(*) as num_sessions from 'sessions.last30d' " +
        "group by TRUNCATE(time / " + time_window * 1000000 +") order by time asc;";
    }
  };

  var getParamMetric = function() {
    return $(".zbase_session_tracking_dashboard z-dropdown.metric").getValue();
  };

  var getMetricTitle = function() {
    return $(
        ".zbase_session_tracking_dashboard z-dropdown.metric [data-selected]")
        .getAttribute("data-title");
  };

  var getParamTimeWindow = function() {
    return $(".zbase_session_tracking_dashboard z-dropdown.time_window").getValue();
  };

  var setParamMetric = function(value) {
    if (value) {
      $(".zbase_session_tracking_dashboard z-dropdown.metric")
        .setValue([value]);
    }
  };

  var setParamTimeWindow = function(value) {
    if (value) {
      $(".zbase_session_tracking_dashboard z-dropdown.time_window")
        .setValue([value]);
    }
  };

  var setParamsFromAndUntil = function(from, until) {
    if (!until) {
      until = DateUtil.getStartOfDay(Date.now());
    }

    if (!from) {
      from = until - DateUtil.millisPerDay * 30;
    }

    $(".zbase_session_tracking_dashboard z-daterangepicker").setValue(
        from, until);
  };

  var paramChanged = function(e) {
    var url = UrlUtil.addOrModifyUrlParams(
      document.location.pathname,
      [{key: this.getAttribute("name"), value: this.getValue()}]);

    $.navigateTo(url);
  };

  return {
    name: "session_tracking_dashboard",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: render
  };

})());

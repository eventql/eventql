ZBase.registerView((function() {
  $.showLoader();
  var query_mgr;
  var chart;

  var load = function(path) {
    query_mgr = EventSourceHandler();

    var page = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_main_tpl");

    var content = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_dashboard_tpl");

    var menu = SessionTrackingMenu(path);
    menu.render($(".zbase_content_pane .session_tracking_sidebar", page));
    $(".zbase_content_pane .session_tracking_content", page).appendChild(content);

    $.handleLinks(page);
    $.replaceViewport(page);

    setParamMetric(UrlUtil.getParamValue(path, "metric"));
    setParamTimeWindow(UrlUtil.getParamValue(path, "time_window"));

    $(".zbase_session_tracking_dashboard z-dropdown.metric")
        .addEventListener("change", paramChanged);
    $(".zbase_session_tracking_dashboard z-dropdown.time_window")
        .addEventListener("change", paramChanged);

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
    renderQueryProgress();
    destroy();

    var query_string = buildQueryString();
    var query = query_mgr.get(
      "sql_query",
      "/api/v1/sql_stream?query=" + encodeURIComponent(query_string));

    query.addEventListener('result', function(e) {
      query_mgr.close("sql_query");
      var data = JSON.parse(e.data);
      renderChart(data.results);
    });

    query.addEventListener('error', function(e) {
      $.fatalError("Server Error");
    });

    query.addEventListener('status', function(e) {
      renderQueryProgress(JSON.parse(e.data));
    });
  };

  var renderQueryProgress = function(progress) {
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
            name: "Sessions",
            values: y_values,
            color: "#3498db"
          }
        ]
      },
      format: '%Y-%m-%d'
    };
    if (getParamTimeWindow() == "3600") {
      chart_config.format += " %H:%M";
    }

    chart = ZBaseC3Chart(chart_config);
    chart.renderTimeseries("dashboard_chart");
    chart.renderLegend($(".zbase_session_tracking_dashboard .chart_legend"));

    $(".zbase_session_tracking_dashboard .query_progress").classList.add("hidden");
    $(".zbase_session_tracking_dashboard .dashboard").classList.remove("hidden");
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

var ReportSQLWidgetDisplay = function(elem, conf) {
  var query_mgr = EventSourceHandler();

  var loadQuery = function() {
    var tpl = $.getTemplate(
      "views/report",
      "zbase_report_sql_widget_main_tpl");

    var result_pane = $(".zbase_report_sql_result_pane", tpl);
    var query = query_mgr.get(
      "report_sql",
      "/api/v1/sql_stream?query=" + encodeURIComponent(conf.query));

    query.addEventListener('result', function(e) {
      query_mgr.close("report_sql");

      var data = JSON.parse(e.data);
      renderQueryResult(result_pane, data.results);
    });

    query.addEventListener('error', function(e) {
      query_mgr.close("report_sql");

      try {
        renderQueryError(result_pane, JSON.parse(e.data).error);
      } catch (e) {
        renderQueryError(result_pane, e.data);
      }
    });

    elem.appendChild(tpl);
  };


  var renderQueryResult = function(result_pane, results) {
    result_pane.innerHTML = "";
    for (var i = 0; i < results.length; i++) {
      //renderResultBar(result_pane, i, results.length > 1);

      switch (results[i].type) {
        case "chart":
          renderResultChart(result_pane, results[i].svg);
          break;
        case "table":
          renderResultTable(result_pane, results[i].rows, results[i].columns)
          break;
      }
    }
  };

  var renderResultBar = function(result_pane, result_index, multiple_results) {
    var bar = document.createElement("div");
    bar.className = "zbase_result_pane_bar";
    bar.setAttribute('data-active', 'active');
    bar.innerHTML ="<label>Result " + (result_index + 1) + "</label>";

    if (multiple_results) {
      bar.className += " clickable";
      bar.addEventListener('click', function() {
        if (this.hasAttribute('data-active')) {
          this.removeAttribute('data-active');
        } else {
          this.setAttribute('data-active', 'active');
        }
      }, false);
    }

    result_pane.appendChild(bar);
  };

  var renderResultChart = function(result_pane, svg) {
    var chart = document.createElement("div");
    chart.className = "zbase_sql_chart";
    chart.innerHTML = svg;
    result_pane.appendChild(chart);
  };

  var renderResultTable = function(result_pane, rows, columns) {
    var table = document.createElement("table");
    table.className = 'z-table';

    innerHTML = "<thead><tr>";
    columns.forEach(function(column) {
      innerHTML += "<th>" + column + "</th>";
    });

    innerHTML += "</tr></thead><tbody>";

    rows.forEach(function(row) {
      innerHTML += "<tr>";
      row.forEach(function(cell) {
        innerHTML += "<td>" + cell + "</td>";
      });
      innerHTML += "</tr>";
    });

    innerHTML += "</tbody>";
    table.innerHTML = innerHTML;
    result_pane.appendChild(table);
  };


  var renderQueryError = function(result_pane, error) {
    var error_msg = $.getTemplate(
        "report-widgets/report-sql-widget",
        "zbase_report_sql_error_msg_tpl");

    $(".error_text", error_msg).innerHTML = error;
    $.replaceContent(result_pane, error_msg);
  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }
  };

  return {
    render: loadQuery,
    destroy: destroy
  };

};

var ReportSQLWidgetEditor = function(elem, conf) {
  var render = function() {
    console.log(elem, conf);
    var tpl = $.getTemplate(
      "views/report",
      "zbase_report_sql_widget_editor_main_tpl");

    elem.appendChild(tpl);
  };

  return {
    render: render
  }
};

ReportWidgetFactory.registerWidget(
    "sql-widget",
    ReportSQLWidgetDisplay,
    ReportSQLWidgetEditor);


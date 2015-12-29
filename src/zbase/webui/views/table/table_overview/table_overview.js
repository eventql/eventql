ZBase.registerView((function() {
  var kPathPrefix = "/a/datastore/tables/";
  var query_mgr;

  var init = function(path) {
    query_mgr = EventSourceHandler();
    load(path);
  };

  var load = function(path) {
    destroy();
    $.showLoader();

    var table_id = path.substr(kPathPrefix.length);
    loadChart(table_id);

    //load schema
    $.httpGet("/api/v1/tables/" + table_id, function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response).table);
      } else {
        renderError(r.statusText, kPathPrefix + table_id);
      }
      $.hideLoader();
    });
  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }
  };

  var loadChart = function(table_id) {
    //FIXME remove limit
    var query_str =
      "select (TRUNCATE(time / 3600000000) * 3600000) as time, count(*) / 3600 " +
      "as num_inserts from '" + table_id + ".last7d' group by " +
      "TRUNCATE(time / 3600000000) order by time asc limit 3600;";

    var query = query_mgr.get(
      "sql_query",
      "/api/v1/sql?format=json_sse&query=" + encodeURIComponent(query_str));

    query.addEventListener('result', function(e) {
      query_mgr.close("sql_query");

      var data = JSON.parse(e.data);
      renderChart(data.results);
    });

    query.addEventListener('query_error', function(e) {
      query_mgr.close("sql_query");
      console.log("query execution failed");
      renderChartError();
    });

    query.addEventListener('error', function(e) {
      query_mgr.close("sql_query");
      console.log("server error while executing query");
      renderChartError();
    });
  };

  var render = function(schema) {
    var page = $.getTemplate(
        "views/table_overview",
        "zbase_table_overview_main_tpl");

    //set table name
    var table_breadcrumb = $(".table_name_breadcrumb", page);
    table_breadcrumb.innerHTML = schema.name;
    table_breadcrumb.href = kPathPrefix + schema.name;
    $(".pagetitle .table_name", page).innerHTML = schema.name;

    //set tab links
    var tabs = page.querySelectorAll("z-tab a");
    for (var i = 0; i < tabs.length; i++) {
      var link = tabs[i].getAttribute("href");
      tabs[i].setAttribute("href", link + schema.name);
    }

    renderSchemaTable($(".table_schema tbody", page), schema.schema.columns, "");

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderSchemaTable = function(tbody, columns, prefix) {
    columns.forEach(function(column) {
      var tr = document.createElement("tr");
      var column_name = prefix + column.name;
      tr.innerHTML = [
        "<td>", column_name, "</td><td>", column.type,
        "</td><td>", column.optional, "</td>"].join("");

      tbody.appendChild(tr);
      if (column.type == "OBJECT") {
        renderSchemaTable(tbody, column.schema.columns, column_name + ".");
      }
    });
  };

  var renderChart = function(data) {
    var x_values = ["x"];
    var y_values = ["inserts"];
    var y_max = 0;


    data[0].rows.forEach(function(row) {
      x_values.push(parseInt(row[0], 10));

      var y_value = parseFloat(row[1]);
      y_values.push(y_value);
      if (y_value > y_max) {
        y_max = y_value;
      }
    });

    hideChartLoader();
    var chart = c3.generate({
      bindto: "#table_overview_chart",
      size: {
        width: $(".zbase_table_overview .inner_chart").offsetWidth
      },
      padding: {
        bottom: 10
      },
      interaction: true,
      data: {
        x: 'x',
        type: "area",
        columns: [
          x_values, y_values
        ],
        colors: {
          "inserts": '#eaf2fa'
        },
        selection: {
          enabled: true
        },
      },
      line: {
        connectNull: false
      },
      axis: {
        x: {
          show: false,
          type: "timeseries",
          padding: {
            left: 0,
            right: 0
          },
          tick: {
            format: "%Y-%m-%d %H:%M:%S",
            count: 10
          }
        },
        y: {
          show: false,
          min: 0,
          tick: {
            count: 3,
            values: [-0.1, (y_max + 10) / 2, y_max + 10]
          }
        }
      },
      legend: {
        show: false
      },
      grid: {
        y: {
          show: true
        }
      },
      point: {
        show: false
      }
    });
  };

  var hideChartLoader = function() {
    $(".zbase_table_overview .chart_container .zbase_loader").classList.add("hidden");
  };

  var renderChartError = function() {
    var error_msg = document.createElement("div");
    error_msg.classList.add("error_msg");
    error_msg.innerHTML = "No data received.";

    $.replaceContent(
      $(".zbase_table_overview .chart_container .inner_chart"),
      error_msg);
  };

  var renderError = function(msg, path) {
    var error_elem = document.createElement("div");
    error_elem.classList.add("zbase_error");
    error_elem.innerHTML = 
        "<span>" +
        "<h2>We're sorry</h2>" +
        "<h1>An error occured.</h1><p>" + msg + "</p> " +
        "<p>Please try it again or contact support if the problem persists.</p>" +
        "<a href='/a/datastore/tables' class='z-button secondary'>" +
        "<i class='fa fa-arrow-left'></i>&nbsp;Back</a>" +
        "<a href='" + path + "' class='z-button secondary'>" + 
        "<i class='fa fa-refresh'></i>&nbsp;Reload</a>" +
        "</span>";

    $.replaceViewport(error_elem);
  };




  return {
    name: "table_overview",
    loadView: function(params) { init(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };
})());

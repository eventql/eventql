var ReportSQLWidgetDisplay = function() {
  var query_mgr = EventSourceHandler();
  var query_str = "";
  var query_result = [];
  var edit_callbacks = [];
  var delete_callbacks = [];

  var render = function(elem, conf) {
    var tpl = $.getTemplate(
      "views/report",
      "zbase_report_sql_widget_main_tpl");

    if (!conf.hasOwnProperty("name")) {
      conf.name = "Unnamed SQL Query";
    }
    $(".report_widget_title", tpl).innerHTML = conf.name;

    var result_pane = $(".zbase_report_sql_result_pane", tpl);
    var delete_confirmation_modal = $("z-modal.delete_confirmation", tpl);

    $(".zbase_report_widget_header z-dropdown", tpl).addEventListener("change", function() {
      switch (this.getValue()) {
        case "edit":
          triggerEdit();
          break;

        case "delete":
          $(".query_name", delete_confirmation_modal).innerHTML = conf.name;
          delete_confirmation_modal.show();
          this.setValue([]);
          break;

        case "open_query":
          openQueryInSQLEditor(conf);
          break;

        default:
          break;
      }
    }, false);

    $.onClick($("button.submit", delete_confirmation_modal), triggerDelete);
    $.onClick($("button.cancel", delete_confirmation_modal), function() {
      delete_confirmation_modal.close();
    });

    $.replaceContent(elem, tpl);

    if (conf.query == query_str) {
      //display cached result
      renderQueryResult(result_pane, query_result);
    } else {
      //load new query
      loadQuery(result_pane, conf);
    }
  };


  var loadQuery = function(result_pane, conf) {
    destroy();

    var query = query_mgr.get(
      "report_sql",
      "/api/v1/sql_stream?query=" + encodeURIComponent(conf.query));

    query.addEventListener('result', function(e) {
      query_mgr.close("report_sql");

      var data = JSON.parse(e.data);
      renderQueryResult(result_pane, data.results);
      query_str = conf.query;
      query_result = data.results;
    });

    query.addEventListener('error', function(e) {
      query_mgr.close("report_sql");
      query_str = "";
      query_result = [];

      try {
        renderQueryError(result_pane, JSON.parse(e.data).error);
      } catch (e) {
        renderQueryError(result_pane, e.data);
      }
    });
  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }
  };

  var onEdit = function(callback) {
    edit_callbacks.push(callback);
  };

  var onDelete = function(callback) {
    delete_callbacks.push(callback);
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

  var openQueryInSQLEditor = function(conf) {
    var postdata = $.buildQueryString({
      name: conf.name,
      type: "sql_query",
      content: conf.query
    });
    var contentdata = $.buildQueryString({
      content: conf.query
    });

    $.showLoader();
    $.httpPost("/api/v1/documents", postdata, function(r) {
      if (r.status == 201) {
        var doc = JSON.parse(r.response);

        $.httpPost("/api/v1/documents/" + doc.uuid, contentdata, function(r) {
          if (r.status == 201) {
            $.navigateTo("/a/sql/" + doc.uuid);
            return;
          } else {
            $.fatalError();
          }
        });
      } else {
        $.fatalError();
      }
    });
  };

  var triggerDelete = function() {
    delete_callbacks.forEach(function(callback) {
      callback();
    });
  };

  var triggerEdit = function() {
    edit_callbacks.forEach(function(callback) {
      callback();
    });
  };


  return {
    render: render,
    destroy: destroy,
    onEdit: onEdit,
    onDelete: onDelete
  };
};

ReportSQLWidgetDisplay.getInitialConfig = function() {
  return {
    type: "sql-widget",
    name: "Unnamed SQL Query",
    uuid: $.uuid(),
    query: ""
  }
};

var ReportSQLWidgetEditor = function(conf) {
  var editor;
  var save_callbacks = [];
  var cancel_callbacks = [];

  var render = function(elem) {
    var tpl = $.getTemplate(
      "views/report",
      "zbase_report_sql_widget_editor_main_tpl");

    // code editor
    editor = $("z-codeeditor", tpl);
    editor.setValue(conf.query);

    //query name
    var title = $(".report_widget_title em", tpl);
    var name_modal = $("z-modal.zbase_report_sql_modal", tpl);
    var name_input = $("input", name_modal);
    var query_name = conf.hasOwnProperty("name")? conf.name : "";
    name_input.value = query_name;
    title.innerHTML = query_name;

    $.onClick(title, function() {
      name_modal.show();
      name_input.focus();
    });

    $.onClick($("button.submit", name_modal), function() {
      query_name = name_input.value;
      name_input.value = query_name;
      title.innerHTML = query_name;
      name_modal.close();
    });

    //save
    $.onClick($("button.save", tpl), function() {
      conf.query = editor.getValue();
      conf.name = query_name;
      triggerSave();
    });

    //cancel
    $.onClick($("button.cancel", tpl), function() {
      triggerCancel();
    });

    elem.appendChild(tpl);
    editor.focus();
  };


  var onSave = function(callback) {
    save_callbacks.push(callback);
  };

  var onCancel = function(callback) {
    cancel_callbacks.push(callback);
  };

  var triggerSave = function() {
    save_callbacks.forEach(function(callback) {
      callback(conf);
    });
  };

  var triggerCancel = function() {
    cancel_callbacks.forEach(function(callback) {
      callback();
    });
  };

  return {
    render: render,
    destroy: function() {},
    onSave: onSave,
    onCancel: onCancel
  }
};



ReportWidgetFactory.registerWidget(
    "sql-widget",
    ReportSQLWidgetDisplay,
    ReportSQLWidgetEditor);


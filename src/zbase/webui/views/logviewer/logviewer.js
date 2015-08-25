ZBase.registerView((function() {

  var kBasePath = "/a/logviewer/";

  var query_mgr;
  var logfiles;
  var next_page_time;
  var pagination_history = [];

  var init = function(params) {
    $.showLoader();
    query_mgr = EventSourceHandler();

    $.httpGet("/api/v1/logfiles", function(r) {
      if (r.status == 200) {
        logfiles = JSON.parse(r.response).logfile_definitions;
        render();
        updateQuery(params.path);
      } else {
        renderError("Server Error");
      }

      $.hideLoader();
    });
  };

  var destroy = function() {
    query_mgr.closeAll();
  };

  var updateQuery = function(url) {
    setQueryParams(url);
    executeQuery();
  }

  var executeQuery = function() {
    showLoadingBar(0);

    var params = getQueryParams();
    params.limit = 100;

    var url = "/api/v1/logfiles/scan?" + $.buildQueryString(params);
    var query = query_mgr.get("logfile_query", url);

    query.addEventListener("message", function(e) {
      var data = JSON.parse(e.data);
      var result = data.result;

      if (result.length > 0) {
        showLoadingBar(data.rows_scanned);
        renderResult(result);
      }

      if (data.status == "finished") {
        query_mgr.close("logfile_query");
        hideLoadingBar();

        next_page_time = result[result.length - 1].time;
      }
    });

    query.addEventListener("error", function(e) {
      query_mgr.close("logfile_query");
      hideLoadingBar();
    });
  };

  var getQueryParams = function() {
    var params = {};

    // param: logfile
    params.logfile = $(".zbase_logviewer z-dropdown.logfile_control").getValue();

    // param: time
    var time_control = $(".zbase_logviewer .time_control");
    params.time =  time_control.getAttribute("data-timestamp");

    // param: filter
    params.filter_type = $(".zbase_logviewer .filter_type_control").getValue();
    params.filter = $(".zbase_logviewer .filter_control").getValue();

    // param: columns
    params.columns = $(".zbase_logviewer .columns_control").getValue();

    return params;
  };

  var setQueryParams = function(url) {
    // param: logfile
    var logfile = UrlUtil.getPath(url).substr(kBasePath.length);
    setLogfileParam(logfile);
    setLogfileName(logfile);

    // param: end time
    var end_time = UrlUtil.getParamValue(url, "time");
    setTimeParam(end_time);

    // param: filter type
    var filter_type = UrlUtil.getParamValue(url, "filter_type");
    var filter = UrlUtil.getParamValue(url, "filter");
    setFilterParam(filter_type, filter);

    // param: columns
    var columns = UrlUtil.getParamValue(url, "columns");
    setColumnsParam(logfile, columns);

    // param: raw
    var raw = UrlUtil.getParamValue(url, "raw");

    //pagination
    setPagination();
    setHistoricalDataHint();
  };

  var setLogfileParam = function(logfile) {
    var dropdown = $(".zbase_logviewer z-dropdown.logfile_control");
    var items = $("z-dropdown-items", dropdown);
    items.innerHTML = "";

    logfiles.forEach(function(logfile) {
      var item = document.createElement("z-dropdown-item");
      item.innerHTML = logfile.name;
      item.setAttribute("data-value", logfile.name);
      items.appendChild(item);
    });

    dropdown.setValue([logfile]);
  };

  var setTimeParam = function(end_time) {
    if (typeof end_time == "string") {
      end_time = parseInt(end_time, 10);
    }

    if (!end_time) {
      end_time = (new Date()).getTime() * 1000;
    }

    var time_control = $(".zbase_logviewer .time_control");
    time_control.setAttribute("data-timestamp", end_time);
    time_control.value = DateUtil.printTimestamp(end_time);
  }

  var setFilterParam = function(filter_type, filter) {
    if (!filter_type) {
      filter_type = "sql";
    }

    $(".zbase_logviewer .filter_type_control").setValue([filter_type]);
    $(".zbase_logviewer .filter_control").setValue(filter);
  }

  var setColumnsParam = function(logfile, columns_str) {
    if (!columns_str) {
      columns_str = "__raw__";
    }

    var dropdown = $(".zbase_logviewer .columns_control");
    var columns = listLogfileColumns(logfile);
    var items = $("z-dropdown-items", dropdown);
    items.innerHTML = "";

    {
      var item = document.createElement("z-dropdown-item");
      item.innerHTML = "<z-checkbox></z-checkbox> " + "Raw Logline";
      item.setAttribute("data-value", "__raw__");
      items.appendChild(item);
    }

    columns.forEach(function(column) {
      var item = document.createElement("z-dropdown-item");
      item.innerHTML = "<z-checkbox></z-checkbox> " + column;
      item.setAttribute("data-value", column);
      items.appendChild(item);
    });

    dropdown.setValue(columns_str.split(","));
  }

  var setLogfileName = function(logfile) {
    $(".zbase_logviewer .logfile_name_crumb").innerHTML = logfile;
  }

  var setPagination = function() {
    if (pagination_history.length == 0) {
      $(".zbase_logviewer .z-pager .prev").setAttribute("data-disabled", true);
    } else {
      $(".zbase_logviewer .z-pager .prev").removeAttribute("data-disabled");
    }
  }

  var setHistoricalDataHint = function() {
    // FIXME: display "showing historical data" message
  }

  var getQueryURL = function(params) {
    var url = kBasePath + params.logfile;
    delete params.logfile;
    url += "?" + $.buildQueryString(params);
    return url;
  }

  var submitControls = function() {
    next_page_time = null;
    pagination_history = [];

    var url = getQueryURL(getQueryParams());
    $.navigateTo(url);
  }

  var goToNextPage = function() {
    if (!next_page_time) {
      return;
    }

    var params = getQueryParams();
    params.time = next_page_time;
    var url = getQueryURL(params);

    pagination_history.push(url);
    $.navigateTo(url);
  };

  var goToPreviousPage = function() {
    var url = pagination_history.pop();

    if (url) {
      $.navigateTo(url);
    }
  };

  var findLogfileDefinition = function(logfile_name) {
    for (var i = 0; i < logfiles.length; ++i) {
      if (logfiles[i].name == logfile_name) {
        return logfiles[i];
      }
    }

    return null;
  }

  var listLogfileColumns = function(logfile_name) {
    var logfile_def = findLogfileDefinition(logfile_name);
    if (!logfile_def) {
      $.fatalError();
      return null;
    }

    var columns = [];
    for (var i = 0; i < logfile_def.source_fields.length; ++i) {
      columns.push(logfile_def.source_fields[i].name);
    }

    for (var i = 0; i < logfile_def.row_fields.length; ++i) {
      columns.push(logfile_def.row_fields[i].name);
    }

    return columns;
  }

  var render = function() {
    var page = $.getTemplate("views/logviewer", "zbase_logviewer_main_tpl");

    $(".logfile_control", page).addEventListener("change", submitControls);
    $(".filter_type_control", page).addEventListener("change", submitControls);
    $(".filter_control", page).addEventListener("change", submitControls);
    $(".columns_control", page).addEventListener("change", submitControls);
    $(".time_control", page).addEventListener("change", submitControls);
    $(".z-pager .next", page).addEventListener("click", goToNextPage);
    $(".z-pager .prev", page).addEventListener("click", goToPreviousPage);

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderResult = function(result) {
    var logtable = LogfileTable(result);
    logtable.render($(".zbase_logviewer .table_container"));
  }

  var renderError = function(error) {
    var error_msg = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_error_msg_tpl");

    $(".error_text", error_msg).innerHTML = error;
    $.replaceContent($(".zbase_logviewer .table_container"), error_msg);
  }

  var showLoadingBar = function(num_rows) {
    $(".zbase_logviewer").classList.add("loading");
    var elem = $(".zbase_logviewer .loglines_loading_bar");
    elem.classList.remove("hidden");
    $(".loading_bar_info", elem).style.visibility = 
        (num_rows > 0 ? "visible" : "hidden");
    $(".num_rows", elem).innerHTML = num_rows;
  };

  var hideLoadingBar = function() {
    $(".zbase_logviewer").classList.remove("loading");
    $(".zbase_logviewer .loglines_loading_bar").classList.add("hidden");
  };

  return {
    name: "logviewer",
    loadView: init,
    unloadView: destroy,
    handleNavigationChange: updateQuery
  };

})());

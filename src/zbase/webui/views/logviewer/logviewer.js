ZBase.registerView((function() {

  var kBasePath = "/a/logviewer/";

  var query_mgr;
  var logfiles;
  var default_end_time = (new Date()).getTime() * 1000;
  var next_page_time;
  var pagination_history = [];
  var datepicker;

  var init = function(params) {
    $.showLoader();
    query_mgr = EventSourceHandler();

    $.httpGet("/api/v1/logfiles", function(r) {
      render();

      if (r.status == 200) {
        logfiles = JSON.parse(r.response).logfile_definitions;
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
    var time = parseInt(time_control.getAttribute("data-timestamp"), 10) * 1000;
    if (time != default_end_time) {
      params.time = time;
    }

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

    // "showing historic data" hint
    var is_historical = end_time && end_time != default_end_time;
    setHistoricalDataHint(is_historical);
  };

  var setLogfileParam = function(logfile) {
    var items = [];
    logfiles.forEach(function(logfile) {
      var item = document.createElement("z-dropdown-item");
      item.innerHTML = logfile.name;
      item.setAttribute("data-value", logfile.name);
      items.push(item);
    });

    var dropdown = $(".zbase_logviewer z-dropdown.logfile_control");
    dropdown.setDropdownItems(items);
    dropdown.setValue([logfile]);
  };

  var setTimeParam = function(end_time) {
    if (typeof end_time == "string") {
      end_time = parseInt(end_time, 10);
    }

    if (!end_time) {
      end_time = default_end_time;
    }

    var time_control = $(".zbase_logviewer .time_control");
    datepicker.setTime(Math.floor(end_time / 1000));
  };

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

    var columns = listLogfileColumns(logfile);
    var items = [];

    {
      var item = document.createElement("z-dropdown-item");
      item.innerHTML = "<z-checkbox></z-checkbox> " + "Raw Logline";
      item.setAttribute("data-value", "__raw__");
      items.push(item);
    }

    columns.forEach(function(column) {
      var item = document.createElement("z-dropdown-item");
      item.innerHTML = "<z-checkbox></z-checkbox> " + column;
      item.setAttribute("data-value", column);
      items.push(item);
    });

    var dropdown = $(".zbase_logviewer .columns_control");
    dropdown.setDropdownItems(items);
    dropdown.setValue(columns_str.split(","));
  }

  var setLogfileName = function(logfile) {
    $(".zbase_logviewer .logfile_name_crumb").innerHTML = logfile;
  }

  var setPagination = function() {
    var prev_elem = $(".zbase_logviewer .z-pager .prev");

    if (pagination_history.length == 0) {
      prev_elem.setAttribute("data-disabled", true);
    } else {
      prev_elem.removeAttribute("data-disabled");
    }
  }

  var setHistoricalDataHint = function(is_historic) {
    var elem = $(".zbase_logviewer .go_to_recent");
    if (is_historic) {
      elem.classList.remove("hidden");
    } else {
      elem.classList.add("hidden");
    }
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

  var goToMostRecentPage = function() {
    next_page_time = null;
    pagination_history = [];
    default_end_time = (new Date()).getTime() * 1000;
    setTimeParam(default_end_time);

    var params = getQueryParams();
    var url = getQueryURL(params);

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
    datepicker = DateTimePicker($(".time_control", page));

    $(".logfile_control", page).addEventListener("change", submitControls);
    $(".filter_type_control", page).addEventListener("change", submitControls);
    $(".columns_control", page).addEventListener("change", submitControls);
    $(".time_control", page).addEventListener("change", submitControls);
    $(".z-pager .next", page).addEventListener("click", goToNextPage);
    $(".z-pager .prev", page).addEventListener("click", goToPreviousPage);
    $(".go_to_recent a", page).addEventListener("click", goToMostRecentPage);
    $(".filter_control", page).addEventListener("change", submitControls);
    $(".time_control", page).addEventListener(
      "z-datetimepicker-change", submitControls);
    $(".filter_control", page).addEventListener("keyup", function(e) {
      if (e.keyCode == 13) submitControls();
    });

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

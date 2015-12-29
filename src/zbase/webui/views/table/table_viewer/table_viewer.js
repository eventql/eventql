ZBase.registerView((function() {
  var path_prefix = "/a/datastore/tables/view/";
  var query_mgr = null;
  var table;
  var inspectors = [];

  var init = function(path) {
    query_mgr = EventSourceHandler();
    load(path);
  };

  var update = function(path) {
    destroy();
    load(path);
  };

  var load = function(path) {
    $.showLoader();
    table = path.split("?")[0].substr(path_prefix.length);

    var tpl = $.getTemplate(
        "views/table_viewer",
        "zbase_table_viewer_tpl");

    //set table name
    var table_link = $(".pagetitle a.table_name", tpl);
    table_link.innerHTML = table;
    table_link.setAttribute("href", path_prefix + table);
    $(".table_name_breadcrumb", tpl).innerHTML = table;

    //set tab links
    var tabs = tpl.querySelectorAll("z-tab a");
    for (var i = 0; i < tabs.length; i++) {
      var link = tabs[i].getAttribute("href");
      tabs[i].setAttribute("href", link + table);
    }

    //init controls
    $(".filter_type_control", tpl).addEventListener("change", submitControls);
    $(".filter_control", tpl).addEventListener("z-search-submit", submitControls);
    $(".limit_control", tpl).addEventListener("z-input-submit", submitControls);
    $.onClick($(".limit_control z-input-icon", tpl), submitControls);
    $(".offset_control", tpl).addEventListener("z-input-submit", submitControls);
    $.onClick($(".offset_control z-input-icon", tpl), submitControls);

    $.onClick($(".limit_display label", tpl), function() {
      displayHidableInput($(".zbase_table_viewer .limit_display"));
    });
    $.onClick($(".offset_display label", tpl), function() {
      displayHidableInput($(".zbase_table_viewer .offset_display"));
    });

    $.handleLinks(tpl);
    $.replaceViewport(tpl);
    $.hideLoader();

    setQueryParams(path);
    executeQuery();
  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }

    if (inspectors) {
      inspectors.forEach(function(inspector) {
        inspector.destroy();
      });
    }
  };

  var executeQuery = function() {
    showLoadingBar();

    var query = query_mgr.get("table_viewer", getQueryUrl());
    var event_counter = 0;

    query.addEventListener("result", function(e) {
      query_mgr.close("table_viewer");
      renderJSONView(e.data, event_counter);
      event_counter++;
    }, false);

    query.addEventListener("progress", function(e) {
      var data = JSON.parse(e.data);
      if (data.status == "finished") {
        query_mgr.close("table_viewer");
        hideLoadingBar();
      }
    }, false);

    query.addEventListener("query_error", function(e) {
      var error = "";
      if (e.data) {
        error = "Error: " + JSON.parse(e.data).error;
      }

      renderError(error)
      query_mgr.close("table_viewer");
    }, false);
  };

  var submitControls = function() {
    $.navigateTo(getViewUrl());
  };

  var getViewUrl = function() {
    return path_prefix + table + "?" + $.buildQueryString(getViewParams());
  };

  var getQueryUrl = function() {
    return "/api/v1/events/scan?" + $.buildQueryString(getQueryParams())
  };

  var getViewParams = function() {
    var params = {};

    //limit param
    params.limit = $(".zbase_table_viewer .limit_control").getValue();

    //offset param
    params.offset = $(".zbase_table_viewer .offset_control").getValue();

    //filter param
    params.filter_type = $(".zbase_table_viewer .filter_type_control").getValue();
    params.filter = $(".zbase_table_viewer .filter_control").getValue();

    return params;
  };

  var getQueryParams = function() {
    var params = getViewParams();
    params.table = table;

    return params;
  };

  var setQueryParams = function(url) {
    // param: filter
    var filter_type = UrlUtil.getParamValue(url, "filter_type");
    var filter = UrlUtil.getParamValue(url, "filter");
    setFilterParam(filter_type, filter);

    //param limit
    var limit = UrlUtil.getParamValue(url, "limit");
    setLimitParam(limit);

    //param offset
    var offset = UrlUtil.getParamValue(url, "offset");
    setOffsetParam(offset);
  };

  var setFilterParam = function(filter_type, filter) {
    if (!filter_type) {
      filter_type = "sql";
    }

    $(".zbase_table_viewer .filter_type_control").setValue([filter_type]);
    $(".zbase_table_viewer .filter_control").setValue(filter);
  };

  var setLimitParam = function(limit) {
    if (!limit) {
      limit = 10;
    }

    $(".zbase_table_viewer .limit_display .limit_value").innerHTML = limit;
    $(".zbase_table_viewer .limit_control").setValue(limit);
  };

  var setOffsetParam = function(offset) {
    if (!offset) {
      offset = 0;
    }

    $(".zbase_table_viewer .offset_display .offset_value").innerHTML = offset;
    $(".zbase_table_viewer .offset_control").setValue(offset);
  };

  var renderJSONView = function(json, event_counter) {
    var elem = document.createElement("div");
    elem.setAttribute("id", "json_" + event_counter);
    $(".zbase_table_viewer .json_viewer .content").appendChild(elem);


    inspectors.push(new InspectorJSON({
        element: 'json_' + event_counter,
        json: json
      }));

    //$("ul li", elem).classList.add("collapsed");
  };

  var renderError = function(msg) {
    var error_elem = document.createElement("div");
    error_elem.classList.add("zbase_error");
    error_elem.innerHTML = 
        "<span>" +
        "<h2>We're sorry</h2>" +
        "<h1>An error occured.</h1><p>" + msg + "</p> " +
        "<p>Please try it again or contact support if the problem persists.</p>" +
        "<a href='/a/datastore/tables' class='z-button secondary'>" +
        "<i class='fa fa-arrow-left'></i>&nbsp;Back</a>" +
        "<a href='" + path_prefix + table + "' class='z-button secondary'>" + 
        "<i class='fa fa-refresh'></i>&nbsp;Reload</a>" +
        "</span>";

    $.replaceViewport(error_elem);
  };

  var showLoadingBar = function() {
    $(".zbase_table_viewer").classList.add("loading");
    $(".zbase_table_viewer .loading_bar").classList.remove("hidden");
  };

  var hideLoadingBar = function() {
    $(".zbase_table_viewer").classList.remove("loading");
    $(".zbase_table_viewer .loading_bar").classList.add("hidden");
  };

  var onLimitInput = function(e) {
    if (e.keyCode == 13) {
      submitControls();
    }
  };

  var displayHidableInput = function(container) {
    hideInput();
    container.classList.add("active");
    $("input", container).focus();

    $(".zbase_table_viewer").addEventListener("click", hideInput);
    $("z-input", container).addEventListener("click", stopHidingInput);
  };

  var hideInput = function() {
    var active_elem = $(".zbase_table_viewer .hidable_input_container.active");

    if (!active_elem) {
      return;
    }

    active_elem.classList.remove("active");
    $(".zbase_table_viewer").removeEventListener("click", hideInput);
    $("z-input", active_elem).removeEventListener("click", stopHidingInput);
  };

  var stopHidingInput = function(e) {
    e.stopPropagation();
  };

  return {
    name: "table_viewer",
    loadView: function(params) { init(params.path); },
    unloadView: destroy,
    handleNavigationChange: update
  };
})());

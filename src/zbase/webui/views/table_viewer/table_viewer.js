ZBase.registerView((function() {
  var path_prefix = "/a/datastore/tables/view/";
  var query_mgr = null;
  var table;

  var load = function(path) {
    $.showLoader();
    query_mgr = EventSourceHandler();

    table = path.substr(path_prefix.length);

    var tpl = $.getTemplate(
        "views/table_viewer",
        "zbase_table_viewer_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", tpl), "/a/datastore/tables");

    var table_link = $(".pagetitle a.table_name", tpl);
    table_link.innerHTML = table;
    table_link.setAttribute("href", path_prefix + table);

    $(".filter_type_control", tpl).addEventListener(
        "change", submitControls);
    $(".filter_control", tpl).addEventListener(
        "z-search-submit", submitControls);

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
  };

  var executeQuery = function() {
    showLoadingBar();

    var query = query_mgr.get("table_viewer", getQueryUrl());
    var event_counter = 0;

    query.addEventListener("result", function(e) {
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

    query.addEventListener("error", function(e) {
      console.log(e);
      $.fatalError();
    }, false);
  };

  var submitControls = function() {
    //$.navigateTo(getViewUrl());
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
    //FIXME
    params.limit = 10;

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
    // param: filter type
    var filter_type = UrlUtil.getParamValue(url, "filter_type");
    var filter = UrlUtil.getParamValue(url, "filter");
    setFilterParam(filter_type, filter);
  };

  var setFilterParam = function(filter_type, filter) {
    if (!filter_type) {
      filter_type = "sql";
    }

    $(".zbase_table_viewer .filter_type_control").setValue([filter_type]);
    $(".zbase_table_viewer .filter_control").setValue(filter);
  };

  var renderJSONView = function(json, event_counter) {
    var elem = document.createElement("div");
    elem.setAttribute("id", "json_" + event_counter);
    $(".zbase_table_viewer .json_viewer").appendChild(elem);


    var inspector = new InspectorJSON({
      element: 'json_' + event_counter,
      json: json
    });

    $("ul li", elem).classList.add("collapsed");
  };

  var showLoadingBar = function() {
    $(".zbase_table_viewer").classList.add("loading");
    $(".zbase_table_viewer .loading_bar").classList.remove("hidden");
  };

  var hideLoadingBar = function() {
    $(".zbase_table_viewer").classList.remove("loading");
    $(".zbase_table_viewer .loading_bar").classList.add("hidden");
  };

  return {
    name: "table_viewer",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };
})());

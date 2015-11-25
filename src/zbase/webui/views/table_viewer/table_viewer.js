ZBase.registerView((function() {
  var query_mgr = null;

  var load = function(path) {
    $.showLoader();
    query_mgr = EventSourceHandler();

    var path_prefix = "/a/datastore/tables/view/";
    var table = path.substr(path_prefix.length);

    var tpl = $.getTemplate(
        "views/table_viewer",
        "zbase_table_viewer_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", tpl), "/a/datastore/tables");

    var table_link = $(".pagetitle a.table_name", tpl);
    table_link.innerHTML = table;
    table_link.setAttribute("href", path_prefix + table);

    $.handleLinks(tpl);
    $.replaceViewport(tpl);
    $.hideLoader();
    showLoadingBar();

    var params = getQueryParams();
    params.table = table;
    var url = "/api/v1/events/scan?" + $.buildQueryString(params);
    var query = query_mgr.get("table_viewer", url);
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
      $.fatalError();
      console.log(e);
    }, false);
  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }
  };

  var getQueryParams = function() {
    var params = {};

    //limit param
    //FIXME
    params.limit = 10;

    //filter param
    params.filter_type = $(".zbase_table_viewer .filter_type_control").getValue();
    params.filter = $(".zbase_table_viewer .filter_control").getValue();

    return params;
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
    $(".zbase_table_viewer .loading_bar").classList.remove("hidden");
  };

  var hideLoadingBar = function() {
    $(".zbase_table_viewer .loading_bar").classList.add("hidden");
  };

  return {
    name: "table_viewer",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };
})());

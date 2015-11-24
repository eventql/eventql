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

    //REMOVEME
    renderJSONView();
    return;
    //REMOVEME END

    var url = "/api/v1/events/scan?table=" + table + "&limit=2";
    var query = query_mgr.get("table_viewer", url);

    query.addEventListener("result", function(e) {
      console.log(e.data);
      query_mgr.close("table_viewer");
      renderJSONView(JSON.parse(e.data));
    }, false);

    query.addEventListener("progress", function(e) {
      console.log("running");
      var data = JSON.parse(e.data);
      QueryProgressWidget.render($(".zbase_table_viewer .inner_view"), data);
      if (data.status == "finished") {
        query_mgr.close("table_viewer");
      }
    }, false);

    query.addEventListener("error", function(e) {
      query_mgr.close("table_viewer");
      console.log("error", e);
      renderError();
    }, false);

  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }
  };

  var renderJSONView = function(json) {
    $(".zbase_table_viewer .inner_view").innerHTML =
        "<div id='table_viewer_json'></div>";

    var inspector = new InspectorJSON({
      element: 'table_viewer_json',
      json: '{"hello":"world"}'
    });
    console.log(inspector);
  };

  var renderError = function() {
    
  };

  return {
    name: "table_viewer",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };
})());

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
    //renderJSONView();
    //REMOVEME END

    var url = "/api/v1/events/scan?table=" + table + "&limit=10";
    var query = query_mgr.get("table_viewer", url);
    QueryProgressWidget.render($(".zbase_table_viewer .loader"), {status: "running"});

    query.addEventListener("result", function(e) {
      //query_mgr.close("table_viewer");
      renderJSONView(e.data);
      console.log("result");
    }, false);

    query.addEventListener("progress", function(e) {
      console.log("progress");
      var data = JSON.parse(e.data);
      QueryProgressWidget.render($(".zbase_table_viewer .loader"), data);
      if (data.status == "finished") {
        query_mgr.close("table_viewer");
      }
    }, false);

    query.addEventListener("error", function(e) {
      $.fatalError();
      console.log(e);
      renderError();
    }, false);
  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }
  };

  var renderJSONView = function(json) {
    var inspector = new InspectorJSON({
      element: 'table_viewer_json',
      json: json
    });

    $(".zbase_table_viewer .loader").classList.add("hidden");
    $(".zbase_table_viewer .json_viewer").classList.remove("hidden");

    //console.log(JSON.parse(json));
  };

  return {
    name: "table_viewer",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };
})());

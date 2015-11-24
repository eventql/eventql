ZBase.registerView((function() {
  var load = function(path) {
    var path_prefix = "/a/datastore/tables/view/";
    var table = path.substr(path_prefix.length);

    render();
  };

  var destroy = function() {

  };

  var render = function() {
    var tpl = $.getTemplate(
        "views/table_viewer",
        "zbase_table_viewer_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", tpl), "/a/datastore/tables");

    $.handleLinks(tpl);
    $.replaceViewport(tpl);
  };

  return {
    name: "table_viewer",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };
})());

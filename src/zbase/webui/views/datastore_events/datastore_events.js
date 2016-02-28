ZBase.registerView((function() {

  var render = function(path) {
    var page = $.getTemplate(
        "views/datastore_events",
        "zbase_datastore_events_list_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), path);

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  return {
    name: "datastore_events",
    loadView: function(params) { render(params.path); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

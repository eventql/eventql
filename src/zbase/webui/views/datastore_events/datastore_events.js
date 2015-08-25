ZBase.registerView((function() {

  var render = function(url) {
    var layout = $.getTemplate("views/datastore", "zbase_datastore_main_tpl");

    var menu = DatastoreMenu();
    menu.render($(".datastore_sidebar", layout));

    var page = $.getTemplate(
        "views/datastore_events",
        "zbase_datastore_events_list_tpl");

    $.replaceContent($(".datastore_viewport", layout), page);
    $.handleLinks(layout);
    $.replaceViewport(layout);
  };

  return {
    name: "datastore_events",
    loadView: function(params) { render(params.url); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

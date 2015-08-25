ZBase.registerView((function() {

  var render = function(url) {
    var layout = $.getTemplate("views/datastore", "zbase_datastore_main_tpl");

    var menu = DatastoreMenu();
    menu.render($(".datastore_sidebar", layout));

    var page = $.getTemplate(
        "views/datastore_logfiles",
        "zbase_datastore_logfiles_list_tpl");

    $.replaceContent($(".datastore_viewport", layout), page);
    $.handleLinks(layout);
    $.replaceViewport(layout);
  };

  return {
    name: "datastore_logfiles",
    loadView: function(params) { render(params.url); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

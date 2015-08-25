ZBase.registerView((function() {

  var render = function(url) {
    var page = $.getTemplate("views/datastore", "zbase_datastore_main_tpl");

    var menu = DatastoreMenu();
    menu.render($(".datastore_sidebar", page));

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  return {
    name: "datastore",
    loadView: function(params) { render(params.url); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

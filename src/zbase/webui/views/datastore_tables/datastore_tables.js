ZBase.registerView((function() {

  var render = function(url) {
    var page = $.getTemplate(
        "views/datastore_tables",
        "zbase_datastore_tables_list_tpl");

    var menu = HomeMenu();
    menu.render($(".zbase_home_menu_sidebar", page));


    $.handleLinks(page);
    $.replaceViewport(page);
  };

  return {
    name: "datastore_tables",
    loadView: function(params) { render(params.url); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

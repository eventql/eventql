ZBase.registerView((function() {

  var render = function(path) {
    var page = $.getTemplate("views/appstore", "zbase_appstore_main_tpl");

    var menu = HomeMenu();
    menu.render($(".zbase_home_menu_sidebar", page));

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  return {
    name: "appstore",
    loadView: function(params) { render(params.path); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

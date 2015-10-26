ZBase.registerView((function() {

  var render = function(path) {
    //redirect to reports overview
    $.navigateTo("/a/reports");
    return;

    var page = $.getTemplate("views/appstore", "zbase_appstore_main_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), path);

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

ZBase.registerView((function() {

  var render = function(path) {
    var page = $.getTemplate("views/settings", "zbase_settings_main_tpl");

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  return {
    name: "settings",
    loadView: function(params) { render(params.path); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

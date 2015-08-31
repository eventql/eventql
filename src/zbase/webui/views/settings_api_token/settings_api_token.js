ZBase.registerView((function() {
  var render = function() {
    console.log("render settings api token");
    var page = $.getTemplate(
        "views/settings_api_token",
        "zbase_settings_api_token_main_tpl");

    var menu = SettingsMenu();
    menu.render($(".zbase_settings_menu_sidebar", page));
  };

  return {
    name: "settings_api_token",
    loadView: function(params) { render(); },
    unloadView: function() {},
    handleNavigationChange: render
  };

})());

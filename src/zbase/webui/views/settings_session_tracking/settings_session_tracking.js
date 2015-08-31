ZBase.registerView((function() {
  var render = function() {
    var page = $.getTemplate(
        "views/settings_session_tracking",
        "zbase_settings_session_tracking_main_tpl");

    var menu = SettingsMenu();
    menu.render($(".zbase_settings_menu_sidebar", page));

    $.handleLinks(page);
    $.replaceViewport(page);
    $.hideLoader();
  };

  return {
    name: "settings_session_tracking",
    loadView: function(params) { render(); },
    unloadView: function() {},
    handleNavigationChange: render
  };

})());

ZBase.registerView((function() {
  var load = function(path) {
    $.showLoader();

    var page = $.getTemplate(
        "views/settings_session_tracking",
        "zbase_session_tracking_main_tpl");


    var menu = SessionTrackingMenu(path);
    menu.render($(".zbase_content_pane .session_tracking_sidebar", page));

    $.handleLinks(page);
    $.replaceViewport(page);
    $.hideLoader();
  };

  return {
    name: "edit_session_tracking_event",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

ZBase.registerView((function() {

  var load = function(path) {
    var page = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_main_tpl");

    var menu = SessionTrackingMenu(path);
    menu.render($(".zbase_content_pane .session_tracking_sidebar", page));

    $(".zbase_content_pane .session_tracking_content", page).innerHTML  = "api_access";

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  return {
    name: "session_tracking_api_access",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

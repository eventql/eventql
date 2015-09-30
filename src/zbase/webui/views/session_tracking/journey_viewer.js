ZBase.registerView((function() {

  var query_mgr = EventSourceHandler();

  var load = function(path) {
    var page = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_main_tpl");

    var content = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_journey_viewer_tpl");

    var menu = SessionTrackingMenu(path);
    menu.render($(".zbase_content_pane .session_tracking_sidebar", page));

    $(".zbase_content_pane .session_tracking_content", page).appendChild(content);

    $.handleLinks(page);
    $.replaceViewport(page);


    //var query = query_mgr.get(
    //    "journey_fetch",
    //    "/api/v1/events/scan?table=sessions&limit=10");

    //query.addEventListener('result', function(e) {
    //  console.log(e);
    //});
  };

  var destroy = function() {
    query_mgr.closeAll();
  };

  return {
    name: "session_tracking_journey_viewer",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

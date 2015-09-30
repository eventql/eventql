ZBase.registerView((function() {

  var query_mgr;

  var load = function(path) {
    if (query_mgr) {
      query_mgr.closeAll();
    }

    query_mgr = EventSourceHandler();

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

    var query = query_mgr.get(
        "journey_fetch",
        "/api/v1/events/scan?table=sessions&limit=10");

    query.addEventListener('result', function(e) {
      renderJourney(JSON.parse(e.data));
    });

    query.addEventListener('progress', function(e) {
      var loading_bar = $(".zbase_user_journey_viewer .journeys .loading_bar");
      console.log(e);
      if (JSON.parse(e.data).status == "finished") {
        query_mgr.close("journey_fetch");
        loading_bar.remove();
      }
    });
  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }
  };

  var renderJourney = function(data) {
    var journey = document.createElement("li");
    console.log(data);
    journey.innerHTML = data.time;
    $(".zbase_user_journey_viewer .journeys").appendChild(journey);
  };

  return {
    name: "session_tracking_journey_viewer",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

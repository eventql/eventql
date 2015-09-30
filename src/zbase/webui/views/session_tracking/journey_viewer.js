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
    console.log(data);
    var journey = document.createElement("li");

    var time = document.createElement("span");
    time.classList.add("time");
    time.innerHTML =
        DateUtil.printTimestampShort(data.event.first_seen_time / 1000) +
        "&mdash;" +
        DateUtil.printTimestampShort(data.event.last_seen_time / 1000);

    journey.appendChild(time);

    for (k in data.event.attr) {
      var attr = document.createElement("span");
      attr.classList.add("event");
      attr.innerHTML = "<i class='fa fa-tag'></i> " + $.escapeHTML(k) + ": <em>"+ $.escapeHTML(data.event.attr[k]) + "</em>";
      journey.appendChild(attr);
      console.log(k);
    }

    for (k in data.event.event) {
      var attr = document.createElement("span");
      attr.classList.add("event");
      attr.innerHTML = "<i class='fa fa-cube'></i> " + $.escapeHTML(k);
      journey.appendChild(attr);
      console.log(k);
    }

    $(".zbase_user_journey_viewer .journeys").appendChild(journey);
  };

  return {
    name: "session_tracking_journey_viewer",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

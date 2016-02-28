ZBase.registerView((function() {

  var query_mgr;

  var load = function(path) {
    if (query_mgr) {
      query_mgr.closeAll();
    }

    query_mgr = EventSourceHandler();

    var page = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_journey_viewer_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), path);

    $.handleLinks(page);
    $.replaceViewport(page);

    var query = query_mgr.get(
        "journey_fetch",
        "/api/v1/events/scan?table=sessions&limit=100");

    query.addEventListener('result', function(e) {
      renderJourney(JSON.parse(e.data));
    });

    query.addEventListener('progress', function(e) {
      var loading_bar = $(".zbase_user_journey_viewer .journeys .loading_bar");
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
    journey.classList.add("journey");

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
      //console.log(k);
    }

    for (k in data.event.event) {
      var attr = document.createElement("span");
      attr.classList.add("event");
      attr.innerHTML = "<i class='fa fa-cube'></i> " + $.escapeHTML(k);
      journey.appendChild(attr);
      //console.log(k);
    }

    $.onClick(journey, function() {
      destroy();
      var journey_detail = JourneyDetail();
      journey_detail.render($(".zbase_user_journey_viewer"), data);
    });

    $(".zbase_user_journey_viewer .journeys").appendChild(journey);
  };

  return {
    name: "session_tracking_journey_viewer",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

ZBase.registerView((function() {
  var load = function() {
    $.showLoader();

    var page = $.getTemplate(
        "views/settings_session_tracking",
        "zbase_session_tracking_main_tpl");

    var content = $.getTemplate(
        "views/settings_session_tracking",
        "zbase_session_tracking_events_tpl");

    var menu = SessionTrackingMenu();
    menu.render($(".zbase_content_pane .session_tracking_sidebar", page));

    $(".zbase_content_pane .session_tracking_content", page).appendChild(content);

    $.httpGet("/api/v1/session_tracking/events", function(r) {
      if(r.status == 200) {
        renderEvents(JSON.parse(r.response).session_events);
        //$.handleLinks(page); //call?
      } else {
        $.fatalError();
      }
      $.hideLoader();
    });

    $.onClick($(".add_session_event .link", page), function() {
      alert("not yet implemented");
    });

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderEvents = function(events) {
    var tbody = $("table.events tbody");
    var tpl = $.getTemplate(
      "views/settings_session_tracking",
      "zbase_settings_session_tracking_event_row_tpl");

    events.forEach(function(ev) {
      var html = tpl.cloneNode(true);
      var event_name = $(".event_name a", html);
      event_name.innerHTML = ev.event;
      $(".event_schema pre", html).innerHTML = ev.schema_debug;

      tbody.appendChild(html);
    });

    $.onClick(tbody, function(e) {
      console.log("open edit event");
    });
  };

  return {
    name: "session_tracking_events",
    loadView: function(params) { load(); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

ZBase.registerView((function() {
  var render = function(path) {
    var page = $.getTemplate(
        "views/settings_session_tracking",
        "zbase_settings_session_tracking_main_tpl");

    var menu = SettingsMenu();
    menu.render($(".zbase_settings_menu_sidebar", page));

    renderView(path);

    $.handleLinks(page);
    $.replaceViewport(page);
    $.hideLoader();
  };

  var renderView = function(path) {
    if (path.indexOf("attributes") > 0) {
      
    } else {
      loadEvents();
    }
  };

  var loadEvents = function() {
    $.httpGet("/api/v1/session_tracking/events", function(r) {
      if(r.status == 200) {
        renderEvents(JSON.parse(r.response).session_events);
      } else {
        $.fatalError();
      }
    });
  };

  var renderEvents = function(events) {
    var tbody = $("table.session_tracking_events tbody");
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
  };

  return {
    name: "settings_session_tracking",
    loadView: function(params) { render(params.path); },
    unloadView: function() {},
    handleNavigationChange: render
  };

})());

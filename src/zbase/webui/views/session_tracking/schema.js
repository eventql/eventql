ZBase.registerView((function() {
  var events_rendered = false;
  var load = function(path) {
    $.showLoader();

    var page = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_main_tpl");

    var content = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_schemas_tpl");

    var menu = SessionTrackingMenu(path);
    menu.render($(".zbase_content_pane .session_tracking_sidebar", page));

    $(".zbase_content_pane .session_tracking_content", page).appendChild(content);

    $.httpGet("/api/v1/session_tracking/events", function(r) {
      if (r.status == 200) {
        renderEvents(JSON.parse(r.response).session_events);
        //$.handleLinks(page); //call?
      } else {
        $.fatalError();
      }
      $.hideLoader();
    });

    $.httpGet("/api/v1/session_tracking/attributes", function(r) {
      if(r.status == 200) {
        renderAttributes(JSON.parse(r.response).session_attributes);
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
    var tbody = $("table.schemas tbody");
    var tpl = $.getTemplate(
      "views/session_tracking",
      "zbase_session_tracking_schema_row_tpl");

    events.forEach(function(ev) {
      var html = tpl.cloneNode(true);
      $(".name", html).innerHTML = ev.event;
      $(".type", html).innerHTML = "Event";

      $.onClick($("tr", html), function() {
        $.navigateTo("/a/session_tracking/settings/schema/events/" + ev.event);
      });

      tbody.appendChild(html);
    });

    events_rendered = true;
  };

  var renderAttributes = function(attrs) {
    if (!events_rendered) {
      window.setTimeout(function() {
        renderAttributes(attrs);
      }, 1);
      return;
    }

    var tbody = $("table.schemas tbody");
    var tpl = $.getTemplate(
      "views/session_tracking",
      "zbase_session_tracking_schema_row_tpl");

    attrs.columns.forEach(function(attr) {
      var html = tpl.cloneNode(true);
      $(".name", html).innerHTML = attr.name;
      $(".type", html).innerHTML = attr.type;

      tbody.appendChild(html);
    });
  };

  return {
    name: "session_tracking_schema",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

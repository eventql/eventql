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


    $.onClick($("button.add", page), renderAdd);

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

  var renderAdd = function() {
    var modal = $(".zbase_session_tracking z-modal.add_to_schema");
    var tpl = $.getTemplate(
      "views/session_tracking",
      "zbase_session_tracking_schema_add_modal_tpl");

    $.onClick($("button.close", tpl), function() {modal.close();});
    $.onClick($("button.submit", tpl), function() {
      var name = $("input", modal).value;
      if (name.length == 0) {
        $(".error_note", modal).classList.remove("hidden");
        return;
      }

      var type = $("z-dropdown", modal).getValue();
      var qstr = "name=" + name + "&type=" + type;

      alert("post new schema item" + qstr);

      //TODO httpPost and redirect to edit_event if type == "event"
    });

    $.replaceContent($(".container", modal), tpl);

    modal.show();
  };

  return {
    name: "session_tracking_schema",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

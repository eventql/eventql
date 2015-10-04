ZBase.registerView((function() {
  var events_rendered = false;
  var cur_path = "";
  var load = function(path) {
    //FIXME destroy
    cur_path = path;
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

    $.onClick($(".link.add", page), renderAdd);

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderEvents = function(events) {
    var tbody = $("table.schemas tbody");
    var tpl = $.getTemplate(
      "views/session_tracking",
      "zbase_session_tracking_schema_event_row_tpl");

    events.forEach(function(ev) {
      var html = tpl.cloneNode(true);
      $(".name", html).innerHTML = ev.event;
      $(".type", html).innerHTML = "Event";


      $.onClick($("tr", html), function() {
        $.navigateTo("/a/session_tracking/settings/schema/events/" + ev.event);
      });

      $("z-dropdown", html).addEventListener("change", function() {
        if (this.getValue() == "edit") {
          $.navigateTo("/a/session_tracking/settings/schema/events/" + ev.event);
        } else {
          displayEventDeleteVerfication(ev);
        }
      }, false);

      tbody.appendChild(html);
    });

    events_rendered = true;
  };

  var renderAttributes = function(attrs) {
    //FIXME destroy timeout
    if (!events_rendered) {
      window.setTimeout(function() {
        renderAttributes(attrs);
      }, 1);
      return;
    }

    var tbody = $("table.schemas tbody");
    var tpl = $.getTemplate(
      "views/session_tracking",
      "zbase_session_tracking_schema_attr_row_tpl");

    attrs.columns.forEach(function(attr) {
      var html = tpl.cloneNode(true);
      $(".name", html).innerHTML = attr.name;
      $(".type", html).innerHTML = attr.type;

      $("z-dropdown", html).addEventListener("click", function() {
        displayAttrDeleteVerfication(attr);
      }, false);

      tbody.appendChild(html);
    });
  };

  var renderAdd = function() {
    var modal = $(".zbase_session_tracking z-modal.add_to_schema");
    var tpl = $.getTemplate(
      "views/session_tracking",
      "zbase_session_tracking_schema_add_modal_tpl");

    var input = $("input", tpl);
    var repeated_field = $(".field.repeated", tpl);

    $("z-dropdown", tpl).addEventListener("change", function() {
      if (this.getValue() == "EVENT") {
        repeated_field.classList.add("not_visible");
      } else {
        repeated_field.classList.remove("not_visible");
      }
    }, false);

    $.onClick($("button.close", tpl), function() {modal.close();});
    $.onClick($("button.submit", tpl), function() {
      if (input.value.length == 0) {
        $(".error_note", modal).classList.remove("hidden");
        return;
      }

      var type = $("z-dropdown", modal).getValue();
      if (type == "EVENT") {
        var ev_name = $.escapeHTML(input.value);
        var url = "/api/v1/session_tracking/events/add_event?event=" + ev_name;

        $.httpPost(url, "", function(r) {
          if (r.status == 201) {
            //redirect to edit event page
            $.navigateTo("/a/session_tracking/settings/schema/events/" + ev_name);
          } else {
            $.fatalError(r.statusText);
          }
        });

      } else {
        var url =
            "/api/v1/session_tracking/attributes/add_attribute?name=" +
            $.escapeHTML(input.value) + "&optional=true&type=" + type + "&repeated=" +
            $("z-checkbox", modal).hasAttribute("data-active");

        $.httpPost(url, "", function(r) {
          if (r.status == 201) {
            load(cur_path);
          } else {
            $.fatalError(r.statusText);
          }
        });
      }
    });

    $.replaceContent($(".container", modal), tpl);

    modal.show();
    input.focus();
  };

  var displayEventDeleteVerfication = function(ev) {
    var modal = $(".zbase_session_tracking z-modal.delete_session_tracking_item");
    $(".z-modal-header .item_name", modal).innerHTML = ev.event;
    $(".z-modal-content .item_name", modal).innerHTML = "the event " + ev.event;

    $("button.close", modal).onclick = function() {modal.close();}
    $("button.submit", modal).onclick = function() {
      var url = "/api/v1/session_tracking/events/remove_event?event=" + ev.event;
      $.httpPost(url, "", function(r) {
        if (r.status == 201) {
          load(cur_path);
        } else {
          $.fatalError(r.statusText);
        }
      });
    };

    modal.show();
  };

  var displayAttrDeleteVerfication = function(attr) {
    var modal = $(".zbase_session_tracking z-modal.delete_session_tracking_item");
    $(".z-modal-header .item_name", modal).innerHTML = attr.name;
    $(".z-modal-content .item_name", modal).innerHTML = "the field " + attr.name;

    $("button.close", modal).onclick = function() {modal.close();}
    $("button.submit", modal).onclick = function() {
      var url =
          "/api/v1/session_tracking/events/remove_attribute?name=" + attr.name;
      $.httpPost(url, "", function(r) {
        if (r.status == 201) {
          load(cur_path);
        } else {
          $.fatalError(r.statusText);
        }
      });
    };

    modal.show();
  };


  return {
    name: "session_tracking_schema",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

ZBase.registerView((function() {
  var path_prefix = "/a/session_tracking/settings/schema/events/";
  var event_name;

  var init = function(path) {
    event_name = path.substr(path_prefix.length);
    load();
  };

  var load = function() {
    $.showLoader();

    var page = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_main_tpl");

    var menu = SessionTrackingMenu(path_prefix);
    menu.render($(".zbase_content_pane .session_tracking_sidebar", page));

    var content = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_edit_event_tpl");

    $(".zbase_content_pane .session_tracking_content", page).appendChild(content);

    var info_url = "/api/v1/session_tracking/event_info?event=" + event_name;
    $.httpGet(info_url, function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response).event.schema.columns);
      } else {
        $.fatalError();
      }
      $.hideLoader();
    });

    $("zbase-breadcrumbs-section .event_name", page).innerHTML = event_name;
    $("h2.pagetitle .event_name", page).innerHTML = event_name;

    $.onClick($(".link.add_field", page), function(e) {renderAdd("")});

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var render = function(fields) {
    renderTable(fields, $(".zbase_session_tracking table.edit_event tbody"));
  };

  var renderTable = function(fields, tbody) {
    var row_tpl = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_edit_event_row_tpl");

    var table_tpl = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_edit_event_table_tpl");

    fields.forEach(function(field) {
      var html = row_tpl.cloneNode(true);
      $(".name", html).innerHTML = field.name;
      $(".type", html).innerHTML = "[" + field.type.toLowerCase() + "]";
      if (field.repeated) {
        $(".repeated", html).innerHTML = "[repeated]";
      }

      $.onClick($(".delete", html), function(e) {
        renderDelete(field.name);
      });

      tbody.appendChild(html);

      if (field.type == "OBJECT") {
        var table = table_tpl.cloneNode(true);
        renderTable(field.schema.columns, $("tbody", table));
        $.onClick($(".add_field", table), function() {
          renderAdd(field.name + ".");
        });
        tbody.appendChild(table);
      }
    });
  };

  var renderAdd = function(field_prefix) {
    var modal = $(".zbase_session_tracking z-modal.add_field");
    var tpl = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_add_event_modal_tpl");

    var input = $("input", tpl);
    input.value = field_prefix;

    $.onClick($("button.submit", tpl), function() {
      if (input.value.length == 0) {
        input.classList.add("error");
        $(".error_note", modal).classList.remove("hidden");
        return;
      }

      var field_data = {
        event: event_name,
        field: $.escapeHTML(input.value),
        type: $("z-dropdown", modal).getValue(),
        repeated: $("z-checkbox", modal).hasAttribute("data-active"),
        optional: true
      };

      var url =
          "/api/v1/session_tracking/events/add_field?" +
          $.buildQueryString(field_data);

      $.httpPost(url, "", function(r) {
        if (r.status == 201) {
          load();
        } else {
          $(".error_field .error_message", modal).innerHTML = r.responseText;
          $(".error_field", modal).classList.remove("hidden");
        }
      });
    });

    $.replaceContent($(".container", modal), tpl);

    modal.show();
    input.focus();
  };

  var renderDelete = function(field_name) {
    var modal = $(".zbase_session_tracking z-modal.delete_field");
    $(".field_name", modal).innerHTML = field_name;
    modal.show();

    $("button.close", modal).onclick = function() {modal.close()};
    $("button.submit", modal).onclick = function(e) {
      var url =
        "/api/v1/session_tracking/events/remove_field?event=" +
        event_name + "&field=" + field_name;

      $.httpPost(url, "", function(r) {
        console.log(r);
        if (r.status == 201) {
          load();
        } else {
          $.fatalError();
        }
      });
    };
  };

  return {
    name: "edit_session_tracking_event",
    loadView: function(params) { init(params.path); },
    unloadView: function() {},
    handleNavigationChange: init
  };
})());

ZBase.registerView((function() {
  var event_name;

  var init = function(path) {
    var path_prefix = "/a/session_tracking/settings/schema/events/";
    event_name = path.substr(path_prefix.length);

    load();
  };

  var load = function() {
    $.showLoader();

    var page = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_main_tpl");

    var menu = SessionTrackingMenu(path);
    menu.render($(".zbase_content_pane .session_tracking_sidebar", page));

    var content = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_edit_event_tpl");

    $("h3 span", content).innerHTML = event_name;
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
      var html = $("tr", row_tpl.cloneNode(true));
      $(".name", html).innerHTML = field.name;
      $(".type", html).innerHTML = "[" + field.type.toLowerCase() + "]";
      tbody.appendChild(html);

      if (field.repeated) {
        var table = table_tpl.cloneNode(true);
        renderTable(field.schema.fieldumns, $("tbody", table));
        tbody.appendChild(table);
      }

      $("z-dropdown", html).addEventListener("change", function() {
        switch (this.getValue()) {
          case "add":
            console.log("add");
            break;

          case "delete":
            renderDelete(field.name);
            break;
        };
        this.setValue([]);
      }, false);

    });
  };

  var renderDelete = function(field_name) {
    //TODO render popup

    var url =
        "/a/session_tracking/events/remove_field?event=" +
        event_name + "field=" + field_name;
    $.httpPost(url, function(r) {
      if (r.status == 201) {
        load();
      } else {
        $.fatalError();
      }
    });
  };


  return {
    name: "edit_session_tracking_event",
    loadView: function(params) { init(params.path); },
    unloadView: function() {},
    handleNavigationChange: init
  };
})());

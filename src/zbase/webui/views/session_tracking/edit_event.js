ZBase.registerView((function() {
  var load = function(path) {
    $.showLoader();

    var path_prefix = "/a/session_tracking/settings/events/";
    var event_name = path.substr(path_prefix.length);
    var info_url = "/api/v1/session_tracking/event_info?event=" + event_name;

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


  var render = function(columns) {
    renderTable(columns, $(".zbase_session_tracking table.edit_event tbody"));
  };

  var renderTable = function(columns, tbody) {
    var row_tpl = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_edit_event_row_tpl");

    var table_tpl = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_edit_event_table_tpl");

    columns.forEach(function(col) {
      var html = $("tr", row_tpl.cloneNode(true));
      $(".name", html).innerHTML = col.name;
      $(".type", html).innerHTML = "[" + col.type.toLowerCase() + "]";
      tbody.appendChild(html);

      if (col.repeated) {
        var table = table_tpl.cloneNode(true);
        renderTable(col.schema.columns, $("tbody", table));
        tbody.appendChild(table);
        console.log(table);
      }
    });
  };


  return {
    name: "edit_session_tracking_event",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

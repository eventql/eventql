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
        render(JSON.parse(r.response).event);
      } else {
        $.fatalError();
      }
      $.hideLoader();
    });

    $.handleLinks(page);
    $.replaceViewport(page);
  };


  var render = function(event_info) {
    console.log(event_info.schema.columns);
  };

  return {
    name: "edit_session_tracking_event",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

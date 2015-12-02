ZBase.registerView((function() {

  var load = function(path) {
    HeaderWidget.setBreadCrumbs([
      {href: "/a/settings", title: "Settings"},
      {href: "/a/session_tracking", title: "User Tracking"}]);

    var page = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_tracking_pixel_tpl");

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  return {
    name: "session_tracking_tracking_pixel",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

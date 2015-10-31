ZBase.registerView((function() {

  var load = function(path) {
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

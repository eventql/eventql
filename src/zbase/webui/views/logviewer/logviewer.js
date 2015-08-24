ZBase.registerView((function() {

  var init = function(params) {
    render();
    executeQuery(params.path);
  };

  var destroy = function() {
  };

  var render = function() {
    var page = $.getTemplate("views/logviewer", "zbase_logviewer_main_tpl");

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var executeQuery = function(url) {

  };

  return {
    name: "logviewer",
    loadView: init,
    unloadView: destroy,
    handleNavigationChange: executeQuery
  };

})());

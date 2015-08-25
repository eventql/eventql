ZBase.registerView((function() {

  var init = function(params) {
    renderLayout();
    openPage(params.path);
  };

  var renderLayout = function() {
    var page = $.getTemplate("views/datastore", "zbase_datastore_main_tpl");
    $.handleLinks(page);
    $.replaceViewport(page);
  }

  var openPage = function(path) {

  }

  return {
    name: "datastore",
    loadView: init,
    unloadView: function() {},
    handleNavigationChange: openPage
  };
})());

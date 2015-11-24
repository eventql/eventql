ZBase.registerView((function() {

  var load = function(path) {

  };

  var destroy = function() {

  };

  return {
    name: "table_viewer",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };
})());

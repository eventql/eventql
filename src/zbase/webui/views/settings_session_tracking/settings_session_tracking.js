ZBase.registerView((function() {
  var load = function() {

  };

  return {
    name: "settings_session_tracking",
    loadView: function(params) { load(); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

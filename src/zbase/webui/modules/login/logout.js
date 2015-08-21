ZBase.registerView((function() {

  var render = function(path) {
    ZBase.util.httpPost("/analytics/api/v1/auth/logout", "", function() {
      ZBase.navigateTo("/a/");
    });
  };

  return {
    name: "logout",
    loadView: function(params) { render(params.path); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

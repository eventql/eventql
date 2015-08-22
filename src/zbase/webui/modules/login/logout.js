ZBase.registerView((function() {

  var render = function(path) {
    ZBase.showLoader();
    ZBase.util.httpPost("/api/v1/auth/logout", "", function() {
      ZBase.util.httpGet("/a/_/c", function(http) {
        ZBase.hideLoader();

        if (http.status != 200) {
          ZBase.fatalError("logout failed");
          return;
        }

        ZBase.updateConfig(JSON.parse(http.responseText));
        ZBase.navigateTo("/a/");
      });
    });
  };

  return {
    name: "logout",
    loadView: function(params) { render(params.path); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

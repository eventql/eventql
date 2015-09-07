ZBase.registerView((function() {

  var render = function(path) {
    $.showLoader();
    $.httpPost("/api/v1/auth/logout", "", function() {
      $.httpGet("/a/_/c", function(http) {
        $.hideLoader();

        if (http.status != 200) {
          $.fatalError("logout failed");
          return;
        }

        ZBase.updateConfig(JSON.parse(http.responseText));
        $.navigateTo("/a/");
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

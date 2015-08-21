ZBase.registerView((function() {

  var render = function(path) {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate("login", "zbase_login_form_tpl");

    console.log(page);
    viewport.innerHTML = "";
    viewport.appendChild(page);
  };

  return {
    name: "login",
    loadView: function(params) { render(params.path); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

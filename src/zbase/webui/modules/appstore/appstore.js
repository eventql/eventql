ZBase.registerView((function() {

  var render = function(path) {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate("appstore", "zbase_appstore_main_tpl");
    ZBase.util.install_link_handlers(page);

    viewport.appendChild(page);
  };

  return {
    name: "appstore",
    loadView: function(params) { render(params.path); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

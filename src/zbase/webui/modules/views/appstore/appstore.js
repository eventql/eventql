ZBase.registerView((function() {

  var render = function(path) {
    var viewport = document.getElementById("zbase_viewport");
    var page = $.getTemplate("appstore", "zbase_appstore_main_tpl");
    $.handleLinks(page);

    viewport.innerHTML = "";
    viewport.appendChild(page);
  };

  return {
    name: "appstore",
    loadView: function(params) { render(params.path); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

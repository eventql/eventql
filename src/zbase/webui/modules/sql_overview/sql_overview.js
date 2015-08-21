ZBase.registerView((function() {

  var render = function(path) {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate("sql_overview", "zbase_sql_overview_main_tpl");
    console.log(page);
    ZBase.util.install_link_handlers(page);

    viewport.innerHTML = "";
    viewport.appendChild(page);
  };

  return {
    name: "sql_overview",

    loadView: function(params) {
      render(params.path);
    },
    unloadView: function() {
    },
    handleNavigationChange: function(url){
    }
  };

})());

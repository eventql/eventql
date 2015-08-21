ZBase.registerView((function() {

  var render = function(path) {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate("sql_editor", "zbase_sql_editor_main_tpl");
    console.log(page);
    ZBase.util.install_link_handlers(page);

    viewport.innerHTML = "";
    viewport.appendChild(page);
  };

  return {
    name: "sql_editor",

    loadView: function(params) {
      render(params.path);
      console.log("load view sql editor", params);
    },
    unloadView: function() {
      console.log("unload view sql editor");
    },
    handleNavigationChange: function(url){
      console.log("handle nav change sql editor");
    }
  };

})());

ZBase.registerView((function() {

  var renderOverview = function() {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate(
      "sql_editor", "zbase_sql_editor_overview_main_tpl");
    ZBase.util.install_link_handlers(page);

    viewport.innerHTML = "";
    viewport.appendChild(page);
  };

  var render = function(path) {
    var path_parts = path.split("/");
    // render view
    if (path_parts.length == 4 && path_parts[3].length > 0) {
      renderSqlEditor();
    } else {
      renderOverview();
    }
  };

  return {
    name: "sql_editor",

    loadView: function(params) {
      render(params.path);
    },
    unloadView: function() {
    },
    handleNavigationChange: function(url){
    }
  };

})());

var SqlEditorWidget = function() {
  

  var render = function(elem) {
    var tpl = $.getTemplate(
      "views/report",
      "zbase_report_sql_editor_widget_main_tpl");

    elem.appendChild(tpl);
  };

  return {
    renderContent: render

  }
};

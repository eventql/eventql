var ReportSQLWidgetDisplay = function(elem, conf) {

  var init = function() {
    var tpl = $.getTemplate(
      "views/report",
      "zbase_report_sql_editor_widget_main_tpl");

    elem.appendChild(tpl);
  };

  var destroy = function() {

  };

  return {
    init: init,
    destroy: destroy
  };

};

var ReportSQLWidgetEditor = null;

ReportWidgetFactory.registerWidget(
    ReportSQLWidgetDisplay,
    ReportSQLWidgetEditor);


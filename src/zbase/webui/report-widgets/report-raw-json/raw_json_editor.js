var ReportRawJsonEditor = function(conf) {

  return {
    render,
    destroy
  }
};

ReportWidgetFactory.registerWidget(
    "raw-json",
    function() {},
    ReportRawJsonEditor);

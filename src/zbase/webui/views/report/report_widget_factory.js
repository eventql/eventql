var ReportWidgetFactory = (function() {
  var widgets = {};

  var loadWidgets = function(required_widgets, on_loaded) {
    var widget_modules = [];
    required_widgets.forEach(function(widget) {
      widget_modules.push("report-widgets/report-" + widget);
    });

    ZBase.loadModules(widget_modules, on_loaded);
  };

  var register = function(widget_type, display, editor) {
    widgets[widget_type] = {display: display, editor: editor};
  };

  var renderWidgetDisplay = function(type, container, conf) {
    var widget_display = widgets[type].display(container, conf);
    widget_display.render();

    return widget_display;
  };


  var renderWidgetEditor = function(type, container, conf) {
    var widget_editor = widgets[type].editor(container, conf);
    widget_editor.render();

    return widget_editor;
  };


  return {
    loadWidgets: loadWidgets,
    registerWidget: register,
    renderWidgetDisplay: renderWidgetDisplay,
    renderWidgetEditor: renderWidgetEditor
  };
})();

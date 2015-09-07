var AddReportWidgetFlow = function(elem) {
  var render = function() {
    //FIXME load all selectable widget types
    modal.show();
  };

  var saveOnAddedCallback = function(callback) {
    add_callbacks.push(callback);
  };

  var loadWidget = function() {
    var widget_type = $(".widget_selection[data-selected", modal)
        .getAttribute("data-widget");

    modal.close();
    $.showLoader();
    ReportWidgetFactory.loadWidgets([widget_type], function() {
      triggerOnAdded(widget_type);
    });
  };

  var triggerOnAdded = function(widget_type) {
    add_callbacks.forEach(function(callback) {
      callback(widget_type);
    });
  };

  var add_callbacks = [];
  var tpl = $.getTemplate(
      "views/report",
      "zbase_report_add_widget_flow_tpl");

  var modal = $("z-modal", tpl);

  elem.appendChild(tpl);
  $.onClick($("button.submit", modal), loadWidget);

  return {
    render: render,
    onWidgetAdded: saveOnAddedCallback
  }
};

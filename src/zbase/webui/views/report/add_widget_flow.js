var AddReportWidgetFlow = function(elem) {
  var render = function() {
    //FIXME load all selectable widget types
    var selectable_widgets = [{type: "sql-widget", name: "SQL Editor"}];

    var tpl = $.getTemplate(
      "views/report",
      "zbase_report_add_widget_flow_selection_tpl");

    var container = $(".widget_selections", modal);
    container.innerHTML = "";

    for (var i = 0; i < selectable_widgets.length; i++) {
      var item = $(".widget_selection", tpl.cloneNode(true));
      if (i == 0) {
        item.setAttribute("data-selected", "selected")
      }

      $(".inner", item).innerHTML = selectable_widgets[i].name;
      item.setAttribute("data-widget", selectable_widgets[i].type);

      $.onClick(item, function() {
        $(".widget_selection[data-selected]", container)
            .removeAttribute("data-selected");
        this.setAttribute("data-selected", "selected");
      });
      container.appendChild(item);
    };

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

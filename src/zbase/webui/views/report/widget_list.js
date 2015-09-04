var WidgetList = function(elem, widget_definitions) {
  var widgets = [];

  widget_definitions.forEach(function(conf) {
    var container = $.getTemplate(
          "views/report",
          "zbase_report_widget_main_tpl");

    var display_obj = ReportWidgetFactory.renderWidgetDisplay(
        conf.type,
        container,
        conf);

    //$.onClick($(".zbase_report_widget_header .edit", elem), function() {
    //  renderEditView(widget);
    //});

    elem.appendChild(container);

    widgets.push({
      container: container,
      display_obj: display_obj
    });
  });

  var destroy = function() {
    for (var i = 0; i < widgets.length; i++) {
      widgets[i].display_obj.destroy();
    }

    widgets = [];
  };

  var getJSON = function() {
    //foreach widget getJson
    //return widgets;
  };

  //var setJSON = function(new_widgets) {
  //  widgets = new_widgets;
  //  var tpl = $.getTemplate(
  //        "views/report",
  //        "zbase_report_widget_main_tpl");

  //  });
  //};

  var setEditable = function(is_editable) {
    for (var i = 0; i < widgets.length; i++) {
      if (is_editable) {
        widgets[i].container.classList.add("editable");
      } else {
        widgets[i].container.classList.remove("editable");
      }
    }
  };

  //var renderEditView = function(widget) {
  //  var tpl = $.getTemplate(
  //      "views/report",
  //      "zbase_report_edit_widget_tpl");

  //  var view = widget.editView;

  //  pane.innerHTML = "";
  //  view.render($(".zbase_report_widget_pane", tpl));
  //  pane.appendChild(tpl);
  //};


  return {
    getJSON: getJSON,
    setEditable: setEditable,
    destroy: destroy
  }
};

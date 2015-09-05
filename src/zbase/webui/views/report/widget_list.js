var WidgetList = function(elem, widget_definitions) {
  var widgets = [];
  var widget_edit_callbacks = [];

  widget_definitions.forEach(function(conf) {
    widgets.push({
      conf: conf,
      container: null,
      display_obj: null
    });
  });


  var render = function() {
    widgets.forEach(function(widget) {
      var container = $(
            ".zbase_report_widget",
            $.getTemplate(
              "views/report",
              "zbase_report_widget_main_tpl"));

      var display_obj = ReportWidgetFactory.renderWidgetDisplay(
          widget.conf.type,
          container,
          widget.conf);

      $.onClick($(".zbase_report_widget_header .edit", container), function() {
        triggerWidgetEdit(widget.conf.uuid);
      });

      elem.appendChild(container);
      widget.container = container;
      widget.display_obj = display_obj;
    });
  };

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

  var getWidgetConfig = function(widget_id) {
    for (var i = 0; i < widgets.length; i++) {
      if (widgets[i].conf.uuid == widget_id) {
        return widgets[i].conf;
      }
    }

    return null;
  };

  var updateWidgetConfig = function(widget_id, conf) {
    for (var i = 0; i < widgets.length; i++) {
      if (widgets[i].conf.uuid == widget_id) {
        widgets[i].conf = conf;
        return;
      }
    }
    //handle error?
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
  var onWidgetEdit = function(callback) {
    widget_edit_callbacks.push(callback);
  };

  var triggerWidgetEdit = function(widget_id) {
    widget_edit_callbacks.forEach(function(callback) {
      callback(widget_id);
    });
  };

  //unrender

  return {
    render: render,
    getJSON: getJSON,
    getWidgetConfig: getWidgetConfig,
    updateWidgetConfig: updateWidgetConfig,
    setEditable: setEditable,
    onWidgetEdit: onWidgetEdit,
    destroy: destroy
  }
};

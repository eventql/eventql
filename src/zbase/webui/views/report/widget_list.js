var WidgetList = function(widget_definitions) {
  var elem;
  var widgets = [];
  var widget_edit_callbacks = [];

  widget_definitions.forEach(function(conf) {
    widgets.push({
      conf: conf,
      container: null,
      display_obj: null
    });
  });


  var render = function(viewport) {
    elem = viewport;
    widgets.forEach(function(widget) {
      if (widget.display_obj) {
        widget.display_obj.destroy();
      }

      widget.container = $.getTemplate(
          "views/report",
          "zbase_report_widget_main_tpl");

      widget.display_obj = ReportWidgetFactory.renderWidgetDisplay(
          widget.conf.type,
          widget.container,
          widget.conf);

      $.onClick($(".zbase_report_widget_header .edit", widget.container), function() {
        triggerWidgetEdit(widget.conf.uuid);
      });

      elem.appendChild(widget.container);
    });
  };

  var destroy = function() {
    for (var i = 0; i < widgets.length; i++) {
      widgets[i].display_obj.destroy();
    }

    widgets = [];
  };

  var getJSON = function() {
    var json = [];
    widgets.forEach(function(widget) {
      json.push(widget.conf);
    });

    return json;
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
    if (elem) {
      if (is_editable) {
        elem.classList.add("editable");
      } else {
        elem.classList.remove("editable");
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

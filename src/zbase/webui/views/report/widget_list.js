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

  var setJSON = function(json) {
    var widget_ids = {};
    widgets.forEach(function(widget) {
      widget_ids[widget.conf.uuid] = true;
    });

    json.forEach(function(widget_conf) {
      if (widget_conf.hasOwnProperty("uuid") &&
          widget_ids.hasOwnProperty(widget_conf.uuid)) {
              updateWidgetConfig(widget_conf.uuid, widget_conf);
              delete widget_ids[widget_conf.uuid];
      } else {
        addNewWidget(widget_conf.type);
      }
    });

    for (var id in widget_ids) {
      destroyWidget(id);
    };
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

  var addNewWidget = function(widget_type) {
    //FIXME loadWidget(widget_type)
    widgets.push({
      conf: ReportWidgetFactory.getWidgetDisplayInitialConf(widget_type),
      container: null,
      display_obj: null
    });
  };

  var destroyWidget = function(widget_id) {
    for (var i = 0; i < widgets.length; i++) {
      if (widgets[i].conf.uuid == widget_id) {
        widgets[i].display_obj.destroy();
        widgets.splice(i, 1);
        return;
      }
    }
  };

  var setEditable = function(is_editable) {
    if (elem) {
      if (is_editable) {
        elem.classList.add("editable");
      } else {
        elem.classList.remove("editable");
      }
    }
  };

  var onWidgetEdit = function(callback) {
    widget_edit_callbacks.push(callback);
  };

  var triggerWidgetEdit = function(widget_id) {
    widget_edit_callbacks.forEach(function(callback) {
      callback(widget_id);
    });
  };

  //TODO unrender

  return {
    render: render,
    getJSON: getJSON,
    setJSON: setJSON,
    getWidgetConfig: getWidgetConfig,
    updateWidgetConfig: updateWidgetConfig,
    addNewWidget: addNewWidget,
    setEditable: setEditable,
    onWidgetEdit: onWidgetEdit,
    destroy: destroy
  }
};

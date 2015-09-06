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

      widget.container = $(
          ".zbase_report_widget",
          $.getTemplate(
              "views/report",
              "zbase_report_widget_main_tpl"));

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

  var setJSON = function(json, callback) {
    var widget_ids = {};
    widgets.forEach(function(widget) {
      widget_ids[widget.conf.uuid] = true;
    });

    // find updated and new widgets
    var new_widgets = [];
    json.forEach(function(widget_conf) {
      if (widget_conf.hasOwnProperty("uuid") &&
          widget_ids.hasOwnProperty(widget_conf.uuid)) {
              updateWidgetConfig(widget_conf.uuid, widget_conf);
              delete widget_ids[widget_conf.uuid];
      } else {
        new_widgets.push(widget_conf);
      }
    });

    // destroy deleted widgets
    for (var id in widget_ids) {
      destroyWidget(id);
    };

    if (new_widgets.length > 0) {
      addNewWidgets(new_widgets, callback);
    } else {
      callback();
    }
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

  //widget already loaded
  var addNewEmptyWidget = function(widget_type) {
    widgets.push({
      conf: ReportWidgetFactory.getWidgetDisplayInitialConf(widget_type),
      container: null,
      display_obj: null
    });
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


  /********************************* PRIVATE **********************************/
  var addNewWidgets = function(new_widgets, callback) {
    //find required widget types
    var required_widgets = [];
    var widget_types = {};
    for (var i = 0; i < new_widgets.length; i++) {
      if (widget_types.hasOwnProperty(new_widgets[i].type)) {
        continue;
      }
      required_widgets.push(new_widgets[i].type);
      widget_types[new_widgets[i].type] = true;
    }

    //load required widgets
    ReportWidgetFactory.loadWidgets(required_widgets, function() {
      new_widgets.forEach(function(widget_conf) {
        if (!widget_conf.hasOwnProperty("uuid")) {
          widget_conf.uuid = $.uuid();
        }
        widgets.push({
          conf: widget_conf,
          container: null,
          display_obj: null
        });
      });

      callback();
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
    addNewEmptyWidget: addNewEmptyWidget,
    setEditable: setEditable,
    onWidgetEdit: onWidgetEdit,
    destroy: destroy
  }
};

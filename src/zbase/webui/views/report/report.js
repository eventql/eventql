ZBase.registerView((function() {
  var widget_list = null;
  var edit_view = null;
  var docsync = null;

  // REMOVEME
  var goToURL = function(path) {
    var kPathPrefix = "/a/reports/";
    var report_id = path.substr(kPathPrefix.length);
    loadReport(report_id);
  }
  // REMOVEME END

  var loadReport = function(report_id) {
    if (widget_list) {
      widget_list.destroy();
      widget_list = null;
    }

    $.showLoader();
    $.httpGet("/api/v1/documents/" + report_id, function(r) {
      if (r.status == 200) {
        var doc = JSON.parse(r.response);
        if (doc.content.length == 0) {
          doc.content = null;
        } else {
          doc.content = JSON.parse(doc.content);
        }

        // load required widgets
        var required_widgets = findRequiredWidgets(doc);
        ReportWidgetFactory.loadWidgets(required_widgets, function() {
          render(doc);
          $.hideLoader();
        });
      } else {
        $.fatalError();
      }
    });
  };

  var destroy = function() {
    if (docsync) {
      docsync.close();
    }

    if (widget_list) {
      widget_list.destroy();
    }

    if (edit_view) {
      edit_view.destroy();
    }
  };

  var render = function(doc) {
    var readonly = !doc.is_writable;
    var page = $.getTemplate(
        "views/report",
        "zbase_report_main_tpl");

    $.handleLinks(page);
    $.replaceViewport(page);

    setReportName(doc.name);

    var description = "";
    if (doc.content != null && doc.content.hasOwnProperty("description")) {
      description = doc.content.description;
    }
    setReportDescription(description);

    var widgets = [];
    if (doc.content != null && doc.content.hasOwnProperty("widgets")) {
      widgets = doc.content.widgets;
    }
    widget_list = WidgetList(widgets);
    widget_list.onWidgetEdit(function(widget_id) {
      showWidgetEditor(widget_id);
    });
    
    //setup docsync
    docsync = DocSync(
        getDocument,
        "/api/v1/documents/" + doc.uuid,
        $(".zbase_report_infobar"));

    // handle display mode
    $(".zbase_report_pane z-dropdown.mode").addEventListener("change", function() {
      widget_list.setEditable(this.getValue() == "editing");
    }, false);

    if (readonly) {
      $(".readonly_hint").classList.remove("hidden");
    } else {
      initNameEditing();
      initDescriptionEditing();
      initShareDocModal(doc.uuid);
      $.onClick($(".zbase_report_pane .link.edit_source"), showRawJSONEditor);

      //add widget flow
      var add_widget_flow = AddReportWidgetFlow(
          $(".zbase_report_pane"));
      add_widget_flow.onWidgetAdded(function(widget_type) {
        if (!widget_list || !docsync) {
          $.fatalError();
          return;
        }
        widget_list.addNewEmptyWidget(widget_type);
        docsync.saveDocument();
        showReportView();
        $.hideLoader();
      });
      $.onClick($(".zbase_report_pane .link.add_widget"), function() {
        add_widget_flow.render();
      });
    }

    showReportView();
  };
  
  var showReportView = function(doc) {
    //showLoader?
    if (edit_view) {
      edit_view.destroy();
    }
    var viewport = $(".zbase_report_viewport");
    viewport.innerHTML = "";

    if (widget_list) {
      widget_list.render(viewport);
      widget_list.setEditable(true);
    } else {
      $.fatalError();
    }
  };

  var showWidgetEditor = function(widget_id) {
    if (!widget_list) {
      $.fatalError();
      return;
    }

    var conf = widget_list.getWidgetConfig(widget_id);
    var editor = ReportWidgetFactory.getWidgetEditor(conf);

    editor.onSave(function(config) {
      if (!docsync) {
        $.fatalError();
        return;
      }
      widget_list.updateWidgetConfig(widget_id, config);
      docsync.saveDocument();
      showReportView();
    });

    editor.onCancel(function() {
      showReportView();
    });

    showEditView(editor);
  };

  var showRawJSONEditor = function() {
    var editor = ReportJSONEditor(getContentJSON());
    showEditView(editor);

    editor.onSave(function(json) {
      if (!widget_list) {
        $.fatalError();
        return;
      }
      var content = JSON.parse(json);
      setReportDescription(content.description);
      $.showLoader();
      widget_list.setJSON(content.widgets, function() {
        if (!docsync) {
          $.fatalError();
          return;
        }
        docsync.saveDocument();
        showReportView();
        $.hideLoader();
      });
    });

    editor.onCancel(showReportView);
  };

  var showEditView = function(editor) {
    edit_view = editor;
    var viewport = $(".zbase_report_viewport");
    viewport.innerHTML = "";
    edit_view.render(viewport);
  };

  //var setEditable = function(is_editable) {
  //  widget_list.setEditable(is_editable);

  //  if (is_editable) {
  //    $(".writable_report_actions").classList.remove("hidden");
  //    $(".zbase_report_pane .report_name").classList.add("editable");
  //    $(".zbase_report_pane .report_description").classList.add("editable");
  //  } else {
  //    $(".writable_report_actions").classList.add("hidden");
  //    $(".zbase_report_pane .report_name").classList.remove("editable");
  //    $(".zbase_report_pane .report_description").classList.remove("editable");
  //  }
  //};

  var initShareDocModal = function(doc_id) {
    var modal = ShareDocModal(
        $(".zbase_report_pane"),
        doc_id,
        "http://zbase.io/a/reports/" + doc_id);

    $.onClick($(".zbase_report_pane .link.share"), function() {
      modal.show();
    });
  };

  var initNameEditing = function() {
    var modal = $(".zbase_report_pane z-modal.rename_report");
    var name_input = $("input.report_name", modal);

    $.onClick($(".zbase_report_pane .report_name.editable"), function() {
      modal.show();
      name_input.focus();
    });

    $.onClick($("button.submit", modal), function() {
      if (!docsync) {
        $.fatalError();
        return;
      }
      setReportName($.escapeHTML(name_input.value));
      docsync.saveDocument();
      modal.close();
    });
  };

  var initDescriptionEditing = function() {
    var modal = $(".zbase_report_pane z-modal.edit_description");
    var textarea = $("textarea.report_description", modal);

    var showModal = function() {
      modal.show();
      textarea.focus();
    };

    $.onClick($(".zbase_report_pane .report_description.editable"), showModal);
    $.onClick($(".zbase_report_pane .link.add_description"), showModal);

    $.onClick($("button.submit", modal), function() {
      if (!docsync) {
        $.fatalError();
        return;
      }
      setReportDescription($.escapeHTML(textarea.value));
      docsync.saveDocument();
      modal.close();
    });
  };

  var setReportName = function(name) {
    var escaped_name = $.escapeHTML(name);
    $("zbase-breadcrumbs-section.report_name").innerHTML = escaped_name;
    $(".zbase_report_pane .report_name").innerHTML = escaped_name;
    $(".zbase_report_pane z-modal input.report_name").value = escaped_name;
  };

  var setReportDescription = function(description) {
    var escaped_description = $.escapeHTML(description);

    //FIXME
    var readonly = false;
    if (!readonly && escaped_description.length == 0) {
      // display edit link
      $(".zbase_report_pane .sidebar_content .link.add_description")
        .classList.remove("hidden");
    } else {
      $(".zbase_report_pane .sidebar_content .link.add_description")
        .classList.add("hidden");
    }

    $(".zbase_report_pane .report_description").innerHTML = escaped_description;
    $(".zbase_report_pane z-modal textarea.report_description").innerHTML =
      escaped_description;
  };

  var getContentJSON = function() {
    var widgets = [];
    if (widget_list) {
      widgets = widget_list.getJSON();
    }

    return JSON.stringify({
        description: $(".zbase_report_pane .report_description").innerText,
        widgets: widgets});
  };

  var getDocument = function() {
    return {
      content: getContentJSON(),
      name: $(".zbase_report_pane input.report_name").value
    };
  };

  var findRequiredWidgets = function(doc) {
    var widgets = [];

    if (doc.content != null && doc.content.hasOwnProperty("widgets")) {
      for (var i = 0; i < doc.content.widgets.length; i++) {
        if (widgets.indexOf(doc.content.widgets[i].type) == -1) {
          widgets.push(doc.content.widgets[i].type);
        }
      }
    }

    return widgets;
  };


  return {
    name: "report",
    loadView: function(params) { goToURL(params.path); },
    unloadView: destroy,
    handleNavigationChange: goToURL
  };

})());

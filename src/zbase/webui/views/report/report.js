ZBase.registerView((function() {
  var widget_list = null;

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

        var required_widgets = findRequiredWidgets(doc);
        ReportWidgetFactory.loadWidgets(required_widgets, function() {
          renderReport(doc);
          $.hideLoader();
        });
      } else {
        $.fatalError();
      }
    });
  };

  var renderReport = function(doc) {
    readonly = !doc.is_writable;
    var page = $.getTemplate(
        "views/report",
        "zbase_report_main_tpl");

    $.handleLinks(page);
    $.replaceViewport(page);

    setReportName(doc.name);

    var mode_dropdown = $(".zbase_report z-dropdown.mode");
    mode_dropdown.classList.remove("hidden");
    mode_dropdown.addEventListener("change", function() {
      setEditable(this.getValue() == "editing");
    }, false);

    if (readonly) {
      $(".readonly_hint").classList.remove("hidden");
    } else {
      initNameEditing();
      initDescriptionEditing();
      initShareDocModal(doc_id);
    }

    widget_list = WidgetList($(".zbase_report_widgets", page), doc.content.widgets);
    widget_list.setEditable(true);
    
    //// setup docsync
    //docsync = DocSync(
    //    getDocument,
    //    "/api/v1/documents/" + doc_id,
    //    $(".zbase_report_infobar"));

    //setEditable(true);

    //init edit actions
    //initContentEditing();
  };

  var findRequiredWidgets = function(doc) {
    var widgets = [];

    if (doc.content.hasOwnProperty("widgets")) {
      for (var i = 0; i < doc.content.widgets.length; i++) {
        if (widgets.indexOf(doc.content.widgets[i].type) == -1) {
          widgets.push(doc.content.widgets[i].type);
        }
      }
    }

    return widgets;
  }

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
        "http://zbase.io/a/report/" + doc_id);

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
      description = $.escapeHTML(textarea.value);
      updateReportContent();
      docsync.saveDocument();
      modal.close();
    });
  };

  //var initContentEditing = function(page) {
  //  var edit_pane = $(".zbase_report_pane .edit_content_pane");
  //  var report_ui = $(".zbase_report_pane .report_ui");

  //  var closeEditPane = function() {
  //    updateReportContent();
  //    edit_pane.classList.add("hidden");
  //    report_ui.classList.remove("hidden");
  //    $(".error_note", edit_pane).classList.add("hidden");
  //  };

  //  $.onClick($(".zbase_report_pane .link.edit"), function() {
  //    report_ui.classList.add("hidden");
  //    edit_pane.classList.remove("hidden");
  //  });

  //  $.onClick($(".submit", edit_pane), function() {
  //    try {
  //      var content = JSON.parse(
  //          $.escapeHTML($("z-codeeditor", edit_pane).getValue()));
  //      description = content.description;
  //      widget_list.setJSON(content.widgets);
  //      docsync.saveDocument();
  //      closeEditPane();
  //    } catch (e) {
  //      $(".error_note", edit_pane).classList.remove("hidden");
  //    }
  //  });
  //  $.onClick($(".close", edit_pane), closeEditPane);
  //};

  var setReportName = function(name) {
    var escaped_name = $.escapeHTML(name);
    $("zbase-breadcrumbs-section.report_name").innerHTML = escaped_name;
    $(".zbase_report_pane .report_name").innerHTML = escaped_name;
    $(".zbase_report_pane z-modal input.report_name").value = escaped_name;
  };

  var setReportDescription = function() {
    var escaped_description = $.escapeHTML(description);

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

  //var setReportContent = function() {
  //  var content = {
  //    description: description,
  //    widgets: widget_list.getJSON()
  //  };

  //  $(".zbase_report_pane .edit_content_pane z-codeeditor").setValue(
  //      JSON.stringify(content));
  //};

  //var updateReportContent = function() {
  //  setReportContent();
  //  setReportDescription();
  //};

  //var getDocument = function() {
  //  var content = {
  //    description: $(".zbase_report_pane .report_description").innerText,
  //    widgets: widget_list.getJSON()
  //  };

  //  return {
  //    content: JSON.stringify(content),
  //    name: $(".zbase_report_pane input.report_name").value
  //  };
  //};

  var destroy = function() {
    //if (docsync) {
    //  docsync.close();
    //}

    if (widget_list) {
      widget_list.destroy();
    }
  };

  return {
    name: "report",
    loadView: function(params) { goToURL(params.path); },
    unloadView: destroy,
    handleNavigationChange: goToURL
  };

})());

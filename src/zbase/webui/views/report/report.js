ZBase.registerView((function() {
  var kPathPrefix = "/a/reports/";
  var docsync;

  var loadReport = function(params) {
    var query_id = params.path.substr(kPathPrefix.length);

    $.showLoader();
    $.httpGet("/api/v1/documents/" + query_id, function(r) {
      if (r.status == 200) {
        var doc = JSON.parse(r.response);
        renderReport(doc);
        $.hideLoader();
      } else {
        $.fatalError();
      }
    });
  };

  var renderReport = function(doc) {
    var readonly = !doc.is_writable;
    var page = $.getTemplate(
        "views/report",
        "zbase_report_main_tpl");

    if (!readonly) {
      // setup docsync
      docsync = DocSync(
          getDocument,
          "/api/v1/documents/" + doc.uuid,
          $(".zbase_report_infobar", page));
    }

    if (!readonly) {
      $(".zbase_report_pane .report_name", page).classList.add("editable");
      $(".zbase_report_pane .report_description", page).classList.add("editable");
    } else {
      $(".readonly_hint", page).classList.remove("hidden");
      $(".writable_report_actions", page).classList.add("hidden");
    }

    $.handleLinks(page);
    $.replaceViewport(page);

    //report name
    setReportName(doc.name);

    //report description
    var description = "";
    try {
      var content = JSON.parse(doc.content);
      description = content.description;
      setReportDescription(content.description, readonly);
    } catch(e) {}
    setReportDescription(description, readonly);

    if (!readonly) {
      //init edit actions
      initNameEditing();
      initDescriptionEditing();
      initShareDocModal(doc.uuid);
      initContentEditing();
    }
  };

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

  var setReportName = function(name) {
    var escaped_name = $.escapeHTML(name);
    $("zbase-breadcrumbs-section.report_name").innerHTML = escaped_name;
    $(".zbase_report_pane .report_name").innerHTML = escaped_name;
    $(".zbase_report_pane z-modal input.report_name").value = escaped_name;
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
      setReportDescription($.escapeHTML(textarea.value));
      docsync.saveDocument();
      modal.close();
    });
  };

  var setReportDescription = function(description, readonly) {
    if (!readonly && description.length == 0) {
      $(".zbase_report_pane .sidebar_content .link.add_description")
        .classList.remove("hidden");
    } else {
      $(".zbase_report_pane .sidebar_content .link.add_description")
        .classList.add("hidden");
    }

    var escaped_description = $.escapeHTML(description);
    $(".zbase_report_pane .report_description").innerHTML = escaped_description;
    $(".zbase_report_pane z-modal textarea.report_description").innerHTML =
      escaped_description;
  };

  var initContentEditing = function(page) {
    var edit_pane = $(".zbase_report_pane .edit_content_pane");
    var report_ui = $(".zbase_report_pane .report_ui");

    var closeEditPane = function() {
      edit_pane.classList.add("hidden");
      report_ui.classList.remove("hidden");
    };

    $.onClick($(".zbase_report_pane .link.edit"), function() {
      report_ui.classList.add("hidden");
      edit_pane.classList.remove("hidden");
    });

    $.onClick($(".close", edit_pane), closeEditPane);
  };

  var getDocument = function() {
    var content = {
      description:
          $(".zbase_report_pane z-modal textarea.report_description").value
    }
    return {
      content: JSON.stringify(content),
      name: $(".zbase_report_pane input.report_name").value
    };
  };

  var destroy = function() {
    if (docsync) {
      docsync.close();
    }
  };

  return {
    name: "report",
    loadView: loadReport,
    unloadView: destroy,
    handleNavigationChange: loadReport
  };

})());

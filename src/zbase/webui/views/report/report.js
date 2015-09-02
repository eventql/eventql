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


    //FIXME stub
    doc.content = {
      description: "Top On-Site Search Terms per Language that were referred by Google"
    };
    //report description
    var report_description = $(".zbase_report_pane .report_description", page);
    report_description.innerHTML = doc.content.description;

    if (!readonly) {
      $(".zbase_report_pane .report_name", page).classList.add("editable");
      report_description.classList.add("editable");
    } else {
      $(".readonly_hint", page).classList.remove("hidden");
      $(".writable_report_actions", page).classList.add("hidden");
    }

    $.handleLinks(page);
    $.replaceViewport(page);

    //report name
    setDocumentName(doc.name);

    if (!readonly) {
      //init edit actions
      initNameEditing(page);
      initDescriptionEditing(page);
      initShareDocModal(page);
      initContentJsonEditing(page);
    }
  };

  var initShareDocModal = function(page) {
  };

  var initNameEditing = function() {
    var modal = $(".zbase_report_pane z-modal.rename_report");
    var name_input = $("input.report_name", modal);

    $.onClick($(".zbase_report_pane .report_name.editable"), function() {
      modal.show();
      name_input.focus();
    });

    $.onClick($("button.submit", modal), function() {
      setDocumentName($.escapeHTML(name_input.value));
      docsync.saveDocument();
      modal.close();
    });

  };

  var setDocumentName = function(name) {
    var escaped_name = $.escapeHTML(name);
    $("zbase-breadcrumbs-section.report_name").innerHTML = escaped_name;
    $(".zbase_report_pane .report_name").innerHTML = escaped_name;
    $(".zbase_report_pane z-modal input.report_name").value = escaped_name;
  };

  var initDescriptionEditing = function(page) {

  };

  var initContentJsonEditing = function(page) {

  };


  var getDocument = function() {
    return {
      content: "",
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

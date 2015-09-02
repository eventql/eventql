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


    //report name
    $(".report_name", page).innerHTML = doc.name;
    var report_title = $(".zbase_report_pane .report_name", page)
    report_title.innerHTML = doc.name;

    //FIXME stub
    doc.content = {
      description: "Top On-Site Search Terms per Language that were referred by Google"
    };
    //report description
    var report_description = $(".zbase_report_pane .report_description", page);
    report_description.innerHTML = doc.content.description;

    if (!readonly) {
      report_title.classList.add("editable");
      report_description.classList.add("editable");

      initShareDocModal();
      initEditing();
    } else {
      $(".readonly_hint", page).classList.remove("hidden");
      $(".writable_report_actions", page).classList.add("hidden");
    }

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var initShareDocModal = function() {

  };

  var initEditing = function() {
    
  };


  var getDocument = function() {

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

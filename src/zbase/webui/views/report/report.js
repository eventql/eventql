ZBase.registerView((function() {
  var kPathPrefix = "/a/reports/";
  var doc_sync;

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

    if (!readonly) {
      report_title.classList.add("editable");
      initNameEditModal();
    }

    //stub
    doc.content = {
      description: "Top On-Site Search Terms per Language that were referred by Google"
    };
    //report description
    $(".zbase_report_pane .report_description", page).innerHTML =
      doc.content.description;



    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var initNameEditModal = function() {

  };

  return {
    name: "report",
    loadView: loadReport,
    unloadView: function() {},
    handleNavigationChange: loadReport
  };

})());

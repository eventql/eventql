ZBase.registerView((function() {

  var load = function(url) {
    var qparams = {
      with_categories: true,
      type: "all",
      author: "all"
    };

    var owner_param = UrlUtil.getParamValue(url, "owner");
    if (owner_param) {
      qparams.owner = owner_param;
    }

    var author_param = UrlUtil.getParamValue(url, "author");
    if (author_param) {
      qparams.author = author_param;
    }

    var category_param = UrlUtil.getParamValue(url, "category");
    if (category_param) {
      qparams.category_prefix = category_param;
    }

    var pstatus_param = UrlUtil.getParamValue(url, "publishing_status");
    if (pstatus_param) {
      qparams.publishing_status = pstatus_param;
    }

    $.showLoader();
    ZBaseMainMenu.show();
    HeaderWidget.setBreadCrumbs([
      {href: "/a/", title: "Datastore"},
      {href: "/a/datastore/queries", title: "Queries"}]);

    $.httpGet("/api/v1/documents?" + $.buildQueryString(qparams), function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response), qparams, url);
      } else {
        $.fatalError();
      }
    });
  }

  var render = function(data, qparams, path) {
    var reports = data.documents;
    var categories = data.categories;

    var page = $.getTemplate(
        "views/datastore_queries",
        "zbase_datastore_queries_main_tpl");

    renderTable(
        page.querySelector(".zbase_datastore_queries tbody"),
        reports);

    $("z-dropdown.create_new_doc", page).addEventListener("change", function(e) {
      $.createNewDocument(this.getValue());
      this.setValue([]);
    });

    $.handleLinks(page);
    $.replaceViewport(page)
    $.hideLoader();
  };

  var renderTable = function(tbody_elem, reports) {
    var doc_types = {
      report: true,
      sql_query: true
    };

    reports.forEach(function(doc) {
      if (!doc_types[doc.type]) {
        return;
      }
      var url = getPathPrefixForDocType(doc.type) + doc.uuid;

      var tr = document.createElement("tr");
      tr.innerHTML = 
          "<td><a href='" + url + "'>" + $.escapeHTML(doc.name) + "</a></td>" +
          "<td><a href='" + url + "'>" + $.escapeHTML(doc.type) + "</a></td>" +
          "<td><a href='" + url + "'>" + DateUtil.printTimeAgo(doc.mtime) + "</a></td>";

      tbody_elem.appendChild(tr);
    });
  };

  var getPathPrefixForDocType = function(doctype) {
    switch (doctype) {
      case "report":
        return "/a/reports/";
      case "sql_query":
        return "/a/sql/";
    }
  }

  return {
    name: "datastore_queries",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

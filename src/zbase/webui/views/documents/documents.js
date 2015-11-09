ZBase.registerView((function() {

  var load = function(url) {
    var qparams = {
      with_categories: true,
      type: "report",
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
    $.httpGet("/api/v1/documents?" + $.buildQueryString(qparams), function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response), qparams, url);
      } else {
        $.fatalError();
      }
    });
  }

  var render = function(data, qparams, path) {
    var documents = data.documents;
    var categories = data.categories;

    var page = $.getTemplate(
        "views/documents",
        "zbase_documents_main_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), path);

    renderDocumentsList(
        page.querySelector(".zbase_documents tbody"),
        documents);

    $.onClick($("button.new_report", page), function(e) {
      $.createNewDocument("report");
    });

    $.handleLinks(page);
    $.replaceViewport(page)
    $.hideLoader();
  };

  var renderDocumentsList = function(tbody_elem, documents) {
    documents.forEach(function(doc) {
      var url = getUrlForDocument(doc);

      var tr = document.createElement("tr");
      tr.innerHTML = 
          "<td><a href='" + url + "'>" + $.escapeHTML(doc.name) + "</a></td>" +
          "<td><a href='" + url + "'>" + $.escapeHTML(doc.type) + "</a></td>" +
          "<td><a href='" + url + "'>" + DateUtil.printTimeAgo(doc.mtime) + "</a></td>";

      tbody_elem.appendChild(tr);
    });
  };

  var getUrlForDocument = function(doc) {
    switch (doc.type) {
      case "sql_query":
        return "/a/sql/" + doc.uuid;

      case "report":
        return "/a/reports/" + doc.uuid;

      case "application":
        return JSON.parse(doc.content).url;

      default:
        return "#";
    }
  };

  return {
    name: "documents",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

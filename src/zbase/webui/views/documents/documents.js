ZBase.registerView((function() {

  var load = function(url) {
    var qparams = {
      with_categories: true,
      owner: "self"
    };

    var owner_param = UrlUtil.getParamValue(url, "owner");
    if (owner_param) {
      qparams.owner = owner_param;
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
        render(JSON.parse(r.response), qparams);
      } else {
        $.fatalError();
      }
    });
  }

  var render = function(data, qparams) {
    var documents = data.documents;
    var categories = data.categories;

    var page = $.getTemplate(
        "views/documents",
        "zbase_documents_main_tpl");

    var menu = DocsMenu(categories, data.num_docs_total, data.num_docs_user);
    menu.render($(".docs_sidebar", page));
    menu.setActiveMenuItem(
        qparams.category_prefix ? qparams.category_prefix :
            qparams.owner == "self" ? "my_documents" : "all_documents");

    renderDocumentsList(
        page.querySelector(".zbase_documents tbody"),
        documents);

    var new_doc_dropdown = $("z-dropdown", page);
    new_doc_dropdown.addEventListener("change", function(e) {
      createNewDocument(e.detail.value);
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

      $.onClick(tr, function() { $.navigateTo(url); });
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

  var createNewDocument = function(doc_type) {
    var name;

    switch (doc_type) {
      case "sql_query":
        name = "Unnamed SQL Query";
        break;

      case "report":
        name = "Unnamed Report";
        break;

      default:
        $.fatalError();
        return;
    }

    var postdata = $.buildQueryString({
      name: name,
      type: doc_type
    });

    $.httpPost("/api/v1/documents", postdata, function(r) {
      if (r.status == 201) {
        var response = JSON.parse(r.response);
        $.navigateTo(getUrlForDocument(response));
        return;
      } else {
        $.fatalError();
      }
    });
  };

  return {
    name: "documents",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

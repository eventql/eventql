ZBase.registerView((function() {

  var load = function() {
    $.showLoader();
    $.httpGet("/api/v1/documents", function(r) {
      if (r.status == 200) {
        var documents = JSON.parse(r.response).documents;
        render(documents);
      } else {
        $.fatalError();
      }
    });
  }

  var render = function(documents) {
    var page = $.getTemplate(
        "views/documents",
        "zbase_documents_main_tpl");

    var menu = HomeMenu();
    menu.render($(".zbase_home_menu_sidebar", page));

    renderDocumentsList(
        page.querySelector(".zbase_documents tbody"),
        documents);

    var new_doc_dropdown = $("z-dropdown", page);
    new_doc_dropdown.addEventListener("z-dropdown-item-click", function(e) {
      var target = e.srcElement || e.target;
      createNewDocument(target.getAttribute("data-type"));
    });

    $.handleLinks(page);
    $.replaceViewport(page)
    $.hideLoader();
  };

  var renderDocumentsList = function(tbody_elem, documents) {
    documents.forEach(function(doc) {
      var url = getUrlForDocument(doc.type, doc.uuid);

      var tr = document.createElement("tr");
      tr.innerHTML = 
          "<td><a href='" + url + "'>" + $.escapeHTML(doc.name) + "</a></td>" +
          "<td><a href='" + url + "'>" + $.escapeHTML(doc.type) + "</a></td>" +
          "<td><a href='" + url + "'>" + DateUtil.printTimeAgo(doc.mtime) + "</a></td>";

      $.onClick(tr, function() { $.navigateTo(url); });
      tbody_elem.appendChild(tr);
    });
  };

  var getUrlForDocument = function(doc_type, uuid) {
    switch (doc_type) {
      case "sql_query":
        return "/a/sql/" + uuid;

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

      default:
        $.fatalError();
    }

    var postdata = $.buildQueryString({
      name: "Unnamed SQL Query",
      type: doc_type
    });

    $.httpPost("/api/v1/documents", postdata, function(r) {
      if (r.status == 201) {
        var response = JSON.parse(r.response);
        $.navigateTo(getUrlForDocument(doc_type, response.uuid));
        return;
      } else {
        $.fatalError();
      }
    });
  };

  return {
    name: "documents",
    loadView: function(params) { load(); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

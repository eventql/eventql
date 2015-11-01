ZBase.registerView((function() {

  var load = function(path) {
    $.showLoader();
    $.httpGet("/api/v1/documents?type=sql_query", function(r) {
      if (r.status == 200) {
        var documents = JSON.parse(r.response).documents;
        render(documents, path);
      } else {
        $.fatalError();
      }
    });
  }

  var render = function(documents, path) {
    var page = $.getTemplate(
        "views/sql_editor",
        "zbase_sql_editor_overview_main_tpl");

    renderDocumentsList(
        page.querySelector(".zbase_sql_editor_overview tbody"),
        documents);

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), path);

    $.onClick($("button[data-action='new-query']", page), function() {
      $.createNewDocument("sql_query");
    });

    $.handleLinks(page);
    $.replaceViewport(page)
    $.hideLoader();
  };

  var renderDocumentsList = function(tbody_elem, documents) {
    documents.forEach(function(doc) {
      var url = "/a/sql/" + doc.uuid;
      var tr = document.createElement("tr");
      tr.innerHTML = 
          "<td><a href='" + url + "'>" + $.escapeHTML(doc.name) + "</a></td>" +
          "<td><a href='" + url + "'>" + $.escapeHTML(doc.type) + "</a></td>" +
          "<td><a href='" + url + "'>" + DateUtil.printTimeAgo(doc.mtime) + "</a></td>";

      $.onClick(tr, function() { $.navigateTo(url); });
      tbody_elem.appendChild(tr);
    });
  };

  return {
    name: "sql_editor_query_list",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

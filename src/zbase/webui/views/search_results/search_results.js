ZBase.registerView((function() {

  var load = function(path) {
    var tpl = $.getTemplate(
        "views/search_results",
        "zbase_search_results_main_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", tpl), path);

    var search_term = UrlUtil.getParamValue(path, "q");

    $.httpGet("/api/v1/documents?search=" + search_term, function(r) {
      if (r.status == 200) {
        var resp = JSON.parse(r.response);

        if (resp.documents.length == 0) {
          console.log(resp);
          renderMessage($(".zbase_search_results .search_results"));
          return;
        }

        if (resp.documents.length == 1 && resp.documents[0].name == search_term) {
          var doc = resp.documents[0];
          //TODO pop search path from history
          $.navigateTo(getUrlForDocType(doc.type) + doc.uuid);
          return;
        }

        renderTable($(".zbase_search_results .search_results"), resp.documents);
      }
    });

    $.replaceViewport(tpl);
  };

  var renderTable = function(elem, docs) {
    var table = $.getTemplate(
        "views/search_results",
        "zbase_search_results_table_tpl");

    var tr_tpl = $.getTemplate(
        "views/search_results",
        "zbase_search_results_row_tpl");

    var tbody = $("tbody", table);


    docs.forEach(function(doc) {
      var tr = tr_tpl.cloneNode(true);
      var url = getUrlForDocType(doc.type) + doc.uuid;

      var name = $(".name", tr);
      name.href = url;
      name.innerHTML = $.escapeHTML(doc.name);

      var type = $(".type", tr);
      type.href = url;
      type.innerHTML = $.escapeHTML(doc.type);

      var modified = $(".modified", tr);
      modified.href = url;
      modified.innerHTML = DateUtil.printTimeAgo(doc.mtime);

      tbody.appendChild(tr);
    });

    $.replaceContent(elem, table);
  };

  var renderMessage = function(elem) {
    var tpl = $.getTemplate(
        "views/search_results",
        "zbase_search_results_message_tpl");

    $.replaceContent(elem, tpl);
  };

  var getUrlForDocType = function(doc_type) {
    switch (doc_type) {
      case "report":
        return "/a/reports/";

      case "sql_query":
        return "/a/sql/";
    }
  };


  return {
    name: "search_results",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

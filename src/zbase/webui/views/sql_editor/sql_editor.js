ZBase.registerView((function() {

  var kPathPrefix = "/a/sql/";

  var query_mgr;
  var docsync;

  var init = function(params) {
    query_mgr = EventSourceHandler();
    loadQuery(params.path);
  };

  var destroy = function() {
    query_mgr.closeAll();

    if (docsync) {
      docsync.close();
    }
  };

  var loadQuery = function(path) {
    var query_id = path.substr(kPathPrefix.length);

    $.showLoader();
    $.httpGet("/api/v1/documents/" + query_id, function(r) {
      if (r.status == 200) {
        var doc = JSON.parse(r.response);
        renderQueryEditor(doc);
        $.hideLoader();
      } else {
        $.fatalError();
      }
    });
  };

  var executeQuery = function(query_string) {
    renderQueryProgress();

    var query = query_mgr.get(
      "sql_query",
      "/api/v1/sql_stream?query=" + encodeURIComponent(query_string));

    query.addEventListener('result', function(e) {
      query_mgr.close("sql_query");

      var data = JSON.parse(e.data);
      renderQueryResult(data.results);
    });

    query.addEventListener('error', function(e) {
      query_mgr.close("sql_query");

      try {
        renderQueryError(JSON.parse(e.data).error);
      } catch (e) {
        renderQueryError(e.data);
      }
    });

    query.addEventListener('status', function(e) {
      renderQueryProgress(JSON.parse(e.data));
    });
  }

  var renderQueryEditor = function(doc) {
    var page = $.getTemplate(
        "views/sql_editor",
        "zbase_sql_editor_main_tpl");

    // setup docsync
    docsync = DocSync(
        getDocument,
        "/api/v1/documents/" + doc.uuid,
        $(".zbase_sql_editor_infobar", page));

    // code editor
    var editor = $("z-codeeditor", page);
    editor.setValue(doc.content);
    editor.addEventListener("execute", function(e) {
      executeQuery(e.value);
      docsync.saveDocument();
    });

    $.onClick($("button[data-action='execute-query']", page), function() {
      editor.execute();
    });

    $.handleLinks(page);
    $.replaceViewport(page);

    // document name + name editing
    setDocumentTitle(doc.name);
    initDocumentNameEditModal();

    // document sharing settings
    initDocumentSharingModal(page);

    // execute query
    if (doc.content.length > 0) {
      executeQuery(doc.content);
    }
  };

  var renderQueryResult = function(res) {
    var result_list = SQLEditorResultList(res);
    result_list.render($(".zbase_sql_editor_result_pane"));
  }

  var renderQueryError = function(error) {
    var error_msg = $.getTemplate(
        "views/sql_editor",
        "zbase_sql_editor_error_msg_tpl");

    $(".error_text", error_msg).innerHTML = error;
    $.replaceContent($(".zbase_sql_editor_result_pane"), error_msg);
  }

  var renderQueryProgress = function(progress) {
    QueryProgressWidget.render(
        $(".zbase_sql_editor_result_pane"),
        progress);
  }

  var initDocumentNameEditModal = function() {
    var modal = $(".zbase_sql_editor_pane z-modal.rename_query");
    var name_input = $(".zbase_sql_editor_pane input[name='doc_name']");

    $.onClick($(".zbase_sql_editor_title h2"), function() {
      modal.show();
      name_input.focus();
    });

    $.onClick($("button[data-action='submit']", modal), function() {
      setDocumentTitle(name_input.value);
      docsync.saveDocument();
      modal.close();
    });
  };

  var initDocumentSharingModal = function(page) {
    var modal = ShareDocModal(page);
  };

  var setDocumentTitle = function(title) {
    $(".zbase_sql_editor_title h2").innerHTML = $.escapeHTML(title);
    $(".sql_document_name_crumb").innerHTML = $.escapeHTML(title);
    $(".zbase_sql_editor_pane input[name='doc_name']").value = title;
  }

  var getDocument = function() {
    return {
      content: $(".zbase_sql_editor_pane z-codeeditor").getValue(),
      name: $(".zbase_sql_editor_pane input[name='doc_name']").value
    };
  }

  return {
    name: "sql_editor",
    loadView: init,
    unloadView: destroy,
    handleNavigationChange: loadQuery
  };

})());

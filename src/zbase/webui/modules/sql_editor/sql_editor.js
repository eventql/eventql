ZBase.registerView((function() {
  var Overview = {};
  var Editor = {};
  var path_parts;

  Editor.render = function() {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate(
      "sql_editor", "zbase_sql_editor_main_tpl");
    ZBase.util.install_link_handlers(page);

    viewport.innerHTML = "";
    viewport.appendChild(page);

    Editor.validateAndFetchDocument();

    Editor.doc_sync = new DocSync(function() {
      return "content=" + encodeURIComponent(document.querySelector(".zbase_sql_editor_pane fn-codeeditor").getValue()) +
      "&name=" + encodeURIComponent(document.querySelector(".zbase_sql_editor_pane input[name='doc_name']").value)
      },
      "/analytics/api/v1/sql_queries/" + Editor.doc_id,
      document.querySelector(".zbase_sql_editor_pane .zbase_sql_editor_infobar")
    );
    Editor.source_handler = new EventSourceHandler();
  };

  Editor.validateAndFetchDocument = function() {
    Editor.doc_id = path_parts[3];
    // invalid docid
    if (!/^[A-Za-z0-9]+$/.test(Editor.doc_id)) {
      //TODO redirect and display message
      alert("invalid docid");
      return;
    }

    // fetch document
    ZBase.util.httpGet("/analytics/api/v1/documents/sql_queries/" + Editor.doc_id, function(r) {
      if (r.status == 200) {
        var doc = JSON.parse(r.response);
        Editor.handleNameEditing(doc.name);
        Editor.setQueryContent(doc.sql_query);
        Editor.observeQueryExecution();

        document.querySelector(".zbase_sql_editor_pane .zbase_loader")
          .classList.add("hidden");
      }
    });
  };

  Editor.setQueryContent = function(query) {
    document.querySelector(".zbase_sql_editor_pane fn-codeeditor").setValue(query);
    if (query != "-- SELECT ... FROM ...;" && query.length > 0) {
      Editor.executeQuery(query);
    }
  };

  Editor.observeQueryExecution = function() {
    var editor = document.querySelector(".zbase_sql_editor_pane fn-codeeditor");
    var exec_btn = document.querySelector(
      ".zbase_sql_editor_pane button[data-action='execute-query']");

    /*document.querySelector("fn-tooltip[data-target='query_exec_btn']").init(
      exec_btn);*/

    exec_btn.addEventListener('click', function() {
      Editor.executeQuery(editor.getValue());
    }, false);

    var cmd_pressed = false;
    editor.addEventListener('keydown', function(e) {
      if (e.keyCode == 17 || e.keyCode == 91) {
        cmd_pressed = true;
      }
    }, false);

    editor.addEventListener('keyup', function(e) {
      if (e.keyCode == 17 || e.keyCode == 91) {
        cmd_pressed = false;
      }
    }, false);

    editor.addEventListener('keydown', function(e) {
      if (e.keyCode == 13 && cmd_pressed) {
        e.preventDefault();
        Editor.executeQuery(editor.getValue());
      }
    }, false);

    var value = editor.getValue();
    editor.addEventListener('keyup', function() {
      var new_value = editor.getValue();
      if (value == new_value) {return;}
      value = new_value;
      /*Editor.doc_sync.cur_version++;
      Editor.doc_sync.documentChanged("content_changed");*/
    }, false);

  };

  Editor.executeQuery = function(query) {
    var id = "query";

    Editor.source_handler.close(id);
    var source = Editor.source_handler.get(
      id,
      "/analytics/api/v1/sql_stream?query=" + encodeURIComponent(query));


    source.addEventListener('result', function(e) {
      var end = (new Date()).getTime();
      Editor.source_handler.close(id);
      var data = JSON.parse(e.data);
      document.querySelector('.zbase_sql_editor_pane .error_message')
        .classList.add("hidden");
      Editor.renderResults(data.results);
    });

    source.addEventListener('error', function(e) {
      Editor.source_handler.close(id);
      //result_pane.style.display = "none";
      try {
        document.querySelector('.zbase_sql_editor_pane .error_text').innerHTML = JSON.parse(e.data).error;
      } catch (e) {
        document.getElementById('.zbase_sql_editor_pane .error_text').innerHTML = e.data;
      }
      document.querySelector('.zbase_sql_editor_pane .error_message')
        .classList.remove("hidden");
    });

  };

  Editor.renderResultTable = function(rows, columns) {
    var table = document.createElement("table");
    table.className = 'zbase_table';

    innerHTML = "<thead><tr>";
    columns.forEach(function(column) {
      innerHTML += "<th>" + column + "</th>";
    });

    innerHTML += "</tr></thead><tbody>";

    rows.forEach(function(row) {
      innerHTML += "<tr>";
      row.forEach(function(cell) {
        innerHTML += "<td>" + cell + "</td>";
      });
      innerHTML += "</tr>";
    });

    innerHTML += "</tbody>";
    table.innerHTML = innerHTML;
    return table;
  };

  Editor.renderResultChart = function(svg) {
    var elem = document.createElement("div");
    elem.className = "zbase_sql_chart";
    elem.innerHTML = svg;
    return elem;
  };

  Editor.renderResultBar = function(index, multiple_results) {
    var bar = document.createElement("div");
    bar.className = "zbase_result_pane_bar";
    bar.setAttribute('data-active', 'active');
    bar.innerHTML =
      "<label>Result " + (index + 1) + "</label>";

    if (multiple_results) {
      bar.className += " clickable";
      bar.addEventListener('click', function() {
        if (this.hasAttribute('data-active')) {
          this.removeAttribute('data-active');
        } else {
          this.setAttribute('data-active', 'active');
        }
      }, false);
    }

    return bar;
  };

  Editor.renderResults = function(results) {
    var result_pane = document.querySelector(
      ".zbase_sql_editor_pane .zbase_sql_editor_result_pane");

    result_pane.innerHTML = "";
    for (var i = 0; i < results.length; i++) {
      result_pane.appendChild(Editor.renderResultBar(i, results.length > 1));
      var elem;
      switch (results[i].type) {
        case "chart":
          elem = Editor.renderResultChart(results[i].svg);
          break;
        case "table":
          elem = Editor.renderResultTable(results[i].rows, results[i].columns)
          break;
      }
      result_pane.appendChild(elem);
    }
  };

  Editor.handleNameEditing = function(doc_name) {
    var name_elem = document.querySelector(".zbase_sql_editor_pane .pagetitle");
    var modal = document.querySelector(".zbase_sql_editor_pane fn-modal");
    var input = modal.querySelector("input");
    name_elem.innerHTML = doc_name;

    name_elem.addEventListener("click", function() {
      input.value = name_elem.innerText;
      modal.show();
      input.focus();
    }, false);
  };

  Overview.render = function() {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate(
      "sql_editor", "zbase_sql_editor_overview_main_tpl");
    ZBase.util.install_link_handlers(page);

    viewport.innerHTML = "";
    viewport.appendChild(page);

    Overview.load();
    Overview.handleNewQueryButton();
  };

  Overview.handleNewQueryButton = function() {
    document.querySelector(
      ".zbase_sql_editor button[data-action='new-query']")
      .addEventListener("click", function() {
        ZBase.util.httpPost("/analytics/api/v1/documents/sql_queries", "", function(r) {
          var response = JSON.parse(r.response);
          window.location.href = "/a/sql/" + response.uuid;
        });
      });
  };

  Overview.renderTable = function(documents) {
    var tbody = document.querySelector(".zbase_sql_editor_overview tbody");
    documents.forEach(function(doc) {
      var url = "/a/sql/" + doc.uuid;
      var tr = document.createElement("tr");
      tr.innerHTML = "<td><a href='" + url + "'>" + doc.name + "</a></td><td>" +
        "<a href='" + url + "'>" + doc.type + "</a></td><td><a href='" + 
        url + "'>&mdash;</a></td>";
      tbody.appendChild(tr);

      tr.addEventListener("click", function() {
        window.location.href = url;
      });
    });

    document.querySelector(".zbase_sql_editor_overview .zbase_loader")
      .classList.add("hidden");
  };

  Overview.load = function() {
    ZBase.util.httpGet("/analytics/api/v1/documents", function(r) {
      if (r.status == 200) {
        var documents = JSON.parse(r.response).documents;
        Overview.renderTable(documents);
      } else {
        //TODO render error message
      }
    });
  };

  var render = function(path) {
    path_parts = path.split("/");
    var view;
    // render view
    if (path_parts.length == 4 && path_parts[3].length > 0) {
      Editor.render();
    } else {
      Overview.render();
    }
  };

  return {
    name: "sql_editor",

    loadView: function(params) {
      render(params.path);
    },
    unloadView: function() {
      if (Editor.source_handler) {
        Editor.source_handler.closeAll();
      }
    },
    handleNavigationChange: function(url){
      render(url);
    }
  };

})());

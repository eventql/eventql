ZBase.registerView((function() {
  var Overview = {};
  var Editor = {};
  var path_parts;

  var source_handler;

  function renderTableList(pane) {
    var ul = pane.querySelector(".zbase_sql_editor_table_list");
    var source_id = "tables";
    var source = source_handler.get(
      source_id,
      "/api/v1/sql_stream?query=" + encodeURIComponent("SHOW TABLES;"));

    source.addEventListener('result', function(e) {
      source_handler.close(source_id);

      var data = JSON.parse(e.data);
      data.results[0].rows.forEach(function(table) {
        var li = document.createElement("li");
        li.innerHTML = table[0];
        ul.appendChild(li);
        li.addEventListener('click', function() {
          renderTableDescriptionModal(table[0]);
        }, false);
      });
      pane.querySelector(".sidebar_section .zbase_loader").classList.add("hidden");
      ul.style.display = "block";
    });
  };

  function renderTableDescriptionModal(table) {
    var modal = document.querySelector(".zbase_sql_editor_modal.table_descr");
    var loader = modal.querySelector(".zbase_loader");
    var source_id = "table_descr";
    var source = source_handler.get(
      source_id,
      "/api/v1/sql_stream?query=" + encodeURIComponent("DESCRIBE '" + table + "';"));

    modal.querySelector(".zbase_modal_header").innerHTML = table;
    loader.classList.remove("hidden");
    modal.show();

    source.addEventListener('result', function(e) {
      source_handler.close(source_id);
      var result = JSON.parse(e.data).results[0];
      var header = modal.querySelector("thead tr");
      var body = modal.querySelector("tbody");

      header.innerHTML = "";
      result.columns.forEach(function(column) {
        header.innerHTML += "<th>" + column + "</th";
      });

      body.innerHTML = "";
      result.rows.forEach(function(row) {
        var html = "<tr>"
        row.forEach(function(cell) {
          html += "<td>" + cell + "</td>";
        });
        html += "</tr>";
        body.innerHTML += html;
      });

      loader.classList.add("hidden");
      modal.querySelector(".error_message").classList.add("hidden");
      modal.querySelector(".zbase_table").classList.remove("hidden");
    });

    source.addEventListener("error", function(e) {
      source_handler.close(source_id);
      var error_message = modal.querySelector(".error_message");
      try {
        error_message.innerHTML = JSON.parse(e.data).error;
      } catch (err) {
        error_message.innerHTML = e.data;
      }

      loader.classList.add("hidden");
      modal.querySelector(".zbase_table").classList.add("hidden");
      error_message.classList.remove("hidden");
    });
  };


  /**
    * SQL Editor
    */
  Editor.render = function() {
    Editor.validateAndFetchDocument();

    Editor.doc_sync = new DocSync(function() {
      return "content=" + encodeURIComponent(document.querySelector(".zbase_sql_editor_pane fn-codeeditor").getValue()) +
      "&name=" + encodeURIComponent(document.querySelector(".zbase_sql_editor_pane input[name='doc_name']").value)
      },
      "/api/v1/sql_queries/" + Editor.doc_id,
      document.querySelector(".zbase_sql_editor_pane .zbase_sql_editor_infobar")
    );

    renderTableList(document.querySelector(".zbase_sql_editor_pane"));
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
    ZBase.util.httpGet("/api/v1/documents/sql_queries/" + Editor.doc_id, function(r) {
      if (r.status == 200) {
        var doc = JSON.parse(r.response);
        Editor.handleNameEditing(doc.name);
        Editor.setQueryContent(doc.sql_query);
        Editor.observeQueryExecution();

        document.querySelector(".zbase_sql_editor_pane .zbase_loader.fullscreen")
          .classList.add("hidden");
      }
    });
  };

  Editor.setQueryContent = function(query) {
    document.querySelector(".zbase_sql_editor_pane fn-codeeditor").setValue(query);
    if (query.length > 0) {
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

    Editor.setResultVisibility("loading");
    source_handler.close(id);
    var source = source_handler.get(
      id,
      "/api/v1/sql_stream?query=" + encodeURIComponent(query));


    source.addEventListener('result', function(e) {
      var end = (new Date()).getTime();
      source_handler.close(id);
      var data = JSON.parse(e.data);
      Editor.renderResults(data.results);
      Editor.setResultVisibility("result");
    });

    source.addEventListener('error', function(e) {
      source_handler.close(id);
      try {
        document.querySelector('.zbase_sql_editor_pane .error_text').innerHTML = JSON.parse(e.data).error;
      } catch (e) {
        document.querySelector('.zbase_sql_editor_pane .error_text').innerHTML = e.data;
      }
      Editor.setResultVisibility("error");
    });

  };

  Editor.setResultVisibility = function(state) {
    switch (state) {
      case "loading":
        document.querySelector('.zbase_sql_editor_result_pane')
          .classList.add("hidden");
        document.querySelector('.zbase_sql_editor_pane .error_message')
          .classList.add("hidden");
        document.querySelector(".zbase_sql_result_loader")
          .classList.remove("hidden");
        return;

      case "error":
        document.querySelector('.zbase_sql_editor_pane .error_message')
          .classList.remove("hidden");
        document.querySelector(".zbase_sql_result_loader")
          .classList.add("hidden");
        return;

      case "result":
        document.querySelector('.zbase_sql_editor_result_pane')
          .classList.remove("hidden");
        document.querySelector(".zbase_sql_result_loader")
          .classList.add("hidden");
      return;

      default:
        break;
    }
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
    var modal = document.querySelector(
      ".zbase_sql_editor_pane fn-modal.rename_query");
    var input = modal.querySelector("input");
    var submit_btn = modal.querySelector("button[data-action='submit']");
    name_elem.innerHTML = doc_name;

    name_elem.addEventListener("click", function() {
      input.value = name_elem.innerText;
      modal.show();
      input.focus();
    }, false);

    input.addEventListener('keyup', function() {
      if (input.value.length == 0) {
        submit_btn.classList.add('disabled');
      } else if (update_btn.classList.contains('disabled')) {
        submit_btn.classList.remove("disabled");
      }
    }, false);

    submit_btn.addEventListener('click', function() {
      var name = input.value;
      if (name.length == 0) {
        return;
      }
      name_elem.innerHTML = name;
      modal.close();
      Editor.doc_sync.cur_version++;
      Editor.doc_sync.documentChanged("content_changed");
    }, false);
  };

  var createNewQuery = function() {
    ZBase.util.httpPost("/api/v1/documents/sql_queries", "", function(r) {
      if (r.status == 201) {
        var response = JSON.parse(r.response);
        ZBase.navigateTo("/a/sql/" + response.uuid);
        return;
      } else {
        //TODO render error message
      }
    });
  };

  var displayQueryEditor = function(query_id) {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate(
      "sql_editor", "zbase_sql_editor_main_tpl");
    ZBase.util.install_link_handlers(page);

    viewport.innerHTML = "";
    viewport.appendChild(page);
  };

  var renderQueryListView = function(documents) {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate(
      "sql_editor", "zbase_sql_editor_overview_main_tpl");

    // render documents list
    var documents_table = page.querySelector(".zbase_sql_editor_overview tbody");
    documents.forEach(function(doc) {
      var url = "/a/sql/" + doc.uuid;
      var tr = document.createElement("tr");
      tr.innerHTML = "<td><a href='" + url + "'>" + doc.name + "</a></td><td>" +
        "<a href='" + url + "'>" + doc.type + "</a></td><td><a href='" + 
        url + "'>&mdash;</a></td>";

      tr.addEventListener("click", function(e) {
        e.preventDefault();
        ZBase.navigateTo(url);
        return false;
      });

      documents_table.appendChild(tr);
    });

    // install new query button click handler
    $.onClick(
        page.querySelector(".zbase_sql_editor button[data-action='new-query']"),
        createNewQuery);

    ZBase.util.install_link_handlers(page);
    viewport.innerHTML = "";
    viewport.appendChild(page);
    ZBase.hideLoader();
  };

  var displayQueryListView = function() {
    ZBase.showLoader();
    ZBase.util.httpGet("/api/v1/documents", function(r) {
      if (r.status == 200) {
        var documents = JSON.parse(r.response).documents;
        renderQueryListView(documents);
      } else {
        //TODO render error message
      }
    });
  }

  var render = function(path) {
    if (path == "/a/sql") {
      displayQueryListView();
    } else {
      var path_prefix = "/a/sql/"
      displayQueryEditor(path.substr(path_prefix.length));
    }
  };

  var init = function() {
    source_handler = new EventSourceHandler();
  };

  var destroy = function() {
    source_handler.closeAll();
  };

  return {
    name: "sql_editor",

    loadView: function(params) {
      init();
      render(params.path);
    },

    unloadView: destroy,
    handleNavigationChange: render
  };

})());

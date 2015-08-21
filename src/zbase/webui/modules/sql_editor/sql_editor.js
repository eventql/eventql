ZBase.registerView((function() {
  var Overview = {};
  var Editor = {};

  Editor.render = function() {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate(
      "sql_editor", "zbase_sql_editor_main_tpl");
    ZBase.util.install_link_handlers(page);

    viewport.innerHTML = "";
    viewport.appendChild(page);

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
    var path_parts = path.split("/");
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
    },
    handleNavigationChange: function(url){
      render(url);
    }
  };

})());

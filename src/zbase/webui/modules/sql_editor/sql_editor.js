ZBase.registerView((function() {

  var renderSqlEditor = function() {
    
  };

  var renderOverview = function() {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate(
      "sql_editor", "zbase_sql_editor_overview_main_tpl");
    ZBase.util.install_link_handlers(page);

    viewport.innerHTML = "";
    viewport.appendChild(page);

    loadOverviewTable();
  };

  var renderOverviewTable = function(documents) {
    var tbody = document.querySelector(".zbase_sql_editor_overview tbody");
    documents.forEach(function(doc) {
      var url = "/a/sql/" + doc.uuid;
      var tr = document.createElement("tr");
      tr.innerHTML = "<td><a href='" + url + "'>" + doc.name + "</td><td>" +
        doc.type + "</td><td>&mdash;</td>";
      tbody.appendChild(tr);

      tr.addEventListener("click", function() {
        window.location.href = url;
      });
    });
  };

  var loadOverviewTable = function() {
    ZBase.util.httpGet("/analytics/api/v1/documents", function(r) {
      console.log(r);
      if (r.status == 200) {
        var documents = JSON.parse(r.response).documents;
        renderOverviewTable(documents);
      } else {
        //TODO render error message
      }
    });
  };

  var render = function(path) {
    var path_parts = path.split("/");
    // render view
    if (path_parts.length == 4 && path_parts[3].length > 0) {
      renderSqlEditor();
    } else {
      renderOverview();
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
    }
  };

})());

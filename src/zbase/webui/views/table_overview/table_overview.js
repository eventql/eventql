ZBase.registerView((function() {
  var kPathPrefix = "/a/datastore/tables/";

  var load = function(path) {
    var table_id = path.substr(kPathPrefix.length);

    $.showLoader();

    $.httpGet("/api/v1/tables/" + table_id, function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response).table);
      } else {
        renderError(r.statusText, kPathPrefix + table_id);
      }
      $.hideLoader();
    });
  };

  var render = function(schema) {
    console.log(schema);
    var page = $.getTemplate(
        "views/table_overview",
        "zbase_table_overview_main_tpl");

    //set table name
    var table_breadcrumb = $(".table_name_breadcrumb", page);
    table_breadcrumb.innerHTML = schema.name;
    table_breadcrumb.href = kPathPrefix + schema.name;
    $(".pagetitle .table_name", page).innerHTML = schema.name;

    //set tab links
    var tabs = page.querySelectorAll("z-tab a");
    for (var i = 0; i < tabs.length; i++) {
      tabs[i].href += schema.name;
      console.log(tabs[i].href);
    }

    renderSchemaTable($(".table_schema tbody", page), schema.schema.columns, "");

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderSchemaTable = function(tbody, columns, prefix) {
    columns.forEach(function(column) {
      var tr = document.createElement("tr");
      var column_name = prefix + column.name;
      tr.innerHTML = [
        "<td>", column_name, "</td><td>", column.type,
        "</td><td>", column.optional, "</td>"].join("");

      tbody.appendChild(tr);
      if (column.type == "OBJECT") {
        renderSchemaTable(tbody, column.schema.columns, column_name + ".");
      }
    });
  };

  var renderError = function(msg, path) {
    var error_elem = document.createElement("div");
    error_elem.classList.add("zbase_error");
    error_elem.innerHTML = 
        "<span>" +
        "<h2>We're sorry</h2>" +
        "<h1>An error occured.</h1><p>" + msg + "</p> " +
        "<p>Please try it again or contact support if the problem persists.</p>" +
        "<a href='/a/datastore/tables' class='z-button secondary'>" +
        "<i class='fa fa-arrow-left'></i>&nbsp;Back</a>" +
        "<a href='" + path + "' class='z-button secondary'>" + 
        "<i class='fa fa-refresh'></i>&nbsp;Reload</a>" +
        "</span>";

    $.replaceViewport(error_elem);
  };




  return {
    name: "table_overview",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

ZBase.registerView((function() {

  var load = function(url) {
    $.showLoader();

    $.httpGet("/api/v1/tables", function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response).tables, url);
      } else {
        renderError(r.statusText);
      }
      $.hideLoader();
    });
  };

  var destroy = function() {
    //abort http request
  };

  var render = function(tables, url) {
    var page = $.getTemplate(
        "views/datastore_tables",
        "zbase_datastore_tables_main_tpl");

    var content = $.getTemplate(
        "views/datastore_tables",
        "zbase_datastore_tables_list_tpl");

    var content_elem = $(".zbase_content_pane", page);
    $.replaceContent(content_elem, content);

    var tbody = $("tbody", page);
    tables.forEach(function(table) {
      renderRow(tbody, table);
    });


    $.onClick($("button.create_table", page), function() {
      createTableView.render(content_elem);
    });

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderRow = function(tbody, table) {
    var elem = $.getTemplate(
        "views/datastore_tables",
        "zbase_datastore_tables_list_row_tpl");

    var table_name = $(".table_name", elem);
    table_name.innerHTML = table.name;

    var edit_links = elem.querySelectorAll("a.edit");
    for (var i = 0; i < edit_links.length; i++) {
      edit_links[i].href = "/a/datastore/tables/" + table.name;
    }

    var viewer_links = elem.querySelectorAll("a.tableviewer");
    for (var i = 0; i < viewer_links.length; i++) {
      viewer_links[i].href = "/a/datastore/tables/view/" + table.name;
    }


    tbody.appendChild(elem);
  };

  var renderError = function(msg) {
    var error_elem = document.createElement("div");
    error_elem.classList.add("zbase_error");
    error_elem.innerHTML = 
        "<span>" +
        "<h2>We're sorry</h2>" +
        "<h1>An error occured.</h1><p>" + msg + "</p> " +
        "<p>Please try it again or contact support if the problem persists.</p>" +
        "<a href='/a/datastore/tables' class='z-button secondary'>" + 
        "<i class='fa fa-refresh'></i>&nbsp;Reload</a>" +
        "</span>";

    $.replaceViewport(error_elem);
  };

  return {
    name: "datastore_tables",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };
})());

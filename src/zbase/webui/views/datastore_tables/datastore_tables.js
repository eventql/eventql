ZBase.registerView((function() {

  var load = function(url) {
    $.showLoader();

    $.httpGet("/api/v1/tables", function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response).tables, url);
      } else {
        $.fatalError();
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

    var content_elem = $(".zbase_content", page);
    $.replaceContent(content_elem, content);

    var tbody = $("tbody", page);
    tables.forEach(function(table) {
      renderRow(tbody, table);
    });

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), url);

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

    var url = "/a/datastore/tables/" + table.name;
    var links = elem.querySelectorAll("a");
    for (var i = 0; i < links.length; i++) {
      links[i].href = url;
    }

    tbody.appendChild(elem);
  };

  return {
    name: "datastore_tables",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };
})());

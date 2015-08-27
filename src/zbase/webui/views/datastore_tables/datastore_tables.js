ZBase.registerView((function() {
  var query_mgr;

  var load = function(url) {
    $.showLoader();
    query_mgr = EventSourceHandler();

    var query = query_mgr.get(
        "tables_list",
        "/api/v1/sql_stream?query=" + encodeURIComponent("SHOW TABLES;"));

    query.addEventListener("result", function(e) {
      query_mgr.close("tables_list");
      render(JSON.parse(e.data).results[0].rows);
      $.hideLoader();
    });

    query.addEventListener("error", function(e) {
      $.fatalError();
      $.hideLoader();
    });

  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }
  };

  var render = function(tables) {
    var page = $.getTemplate(
        "views/datastore_tables",
        "zbase_datastore_tables_list_tpl");

    var menu = HomeMenu();
    menu.render($(".zbase_home_menu_sidebar", page));

    var tbody = $("tbody", page);
    tables.forEach(function(table) {
      renderRow(tbody, table);
    });


    $.handleLinks(page);
    $.replaceViewport(page);
  };


  var renderRow = function(tbody, table) {
    var elem = $.getTemplate(
        "views/datastore_tables",
        "zbase_datastore_tables_list_row_tpl");

    var url = "/a/table/" + table[0];

    $(".table_name", elem).innerHTML = table[0];
    $(".table_description", elem).innerHTML = table[1];

    tbody.appendChild(elem);
  };


  return {
    name: "datastore_tables",
    loadView: function(params) { load(params.url); },
    unloadView: destroy,
    handleNavigationChange: render
  };
})());

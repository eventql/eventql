ZBase.registerView((function() {

  var load = function(url) {
    $.showLoader();

    $.httpGet("/api/v1/tables", function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response).tables);
      } else {
        $.fatalError();
      }
      $.hideLoader();
    });
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

    var url = "/a/table/" + table.name;

    var table_name = $(".table_name", elem);
    table_name.innerHTML = table.name;
    table_name.href = url;
    //$(".table_description", elem).innerHTML = table[1];

    tbody.appendChild(elem);
  };


  return {
    name: "datastore_tables",
    loadView: function(params) { load(params.url); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

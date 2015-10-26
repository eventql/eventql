ZBase.registerView((function() {
  var kPathPrefix = "/a/datastore/tables/";

  var load = function(path) {
    var table_id = path.substr(kPathPrefix.length);

    $.showLoader();
    $.httpGet("/api/v1/tables/" + table_id, function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response).table);
      } else {
        $.fatalError();
      }
      $.hideLoader();
    });
  };

  var render = function(schema) {
    var page = $.getTemplate(
        "views/table",
        "zbase_datastore_table_detail_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), kPathPrefix);

    var table_breadcrumb = $("zbase-breadcrumbs-section.table_name a", page);
    table_breadcrumb.innerHTML = schema.name;
    table_breadcrumb.href = kPathPrefix + schema.name;
    $(".pagetitle em", page).innerHTML = schema.name;

    var tbody = $("tbody", page);

    schema.columns.forEach(function(column) {
      var tr = document.createElement("tr");

      var name_td = document.createElement("td");
      name_td.innerHTML = column.column_name;
      tr.appendChild(name_td);

      var type_td = document.createElement("td");
      type_td.innerHTML = column.type;
      tr.appendChild(type_td);

      var nullable_td = document.createElement("td");
      nullable_td.innerHTML = column.is_nullable;
      tr.appendChild(nullable_td);

      tbody.appendChild(tr);
    });

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  return {
    name: "table",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

var createTableView = (function() {
  var render = function(elem) {
    var tpl = $.getTemplate(
        "views/datastore_table",
        "zbase_datastore_tables_create_table_tpl");

    var columns = $(".columns tbody", tpl);
    renderColumnRow(columns);
    renderColumnRow(columns);

    $.onClick($(".add_column.main", tpl), function() {
      renderColumnRow(columns);
    });

    $.replaceContent(elem, tpl);
  };

  var renderColumnRow = function(elem) {
    var tpl = $.getTemplate(
        "views/datastore_table",
        "zbase_datastore_tables_create_table_column_tpl");

    elem.appendChild(tpl);
  };

  return {
    render: render
  }
})();

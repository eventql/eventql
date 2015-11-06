var createTableView = (function() {
  var render = function(elem) {
    var tpl = $.getTemplate(
        "views/datastore_table",
        "zbase_datastore_tables_create_table_tpl");

    var columns = $(".columns tbody", tpl);

    $.onClick($(".add_column", tpl), function() {
      renderColumnRow(columns);
    });

    renderColumnRow(columns);
    renderColumnRow(columns);

    $.replaceContent(elem, tpl);

    $(".create_table .table input").focus();
  };

  var renderColumnRow = function(elem) {
    var tpl = $.getTemplate(
        "views/datastore_table",
        "zbase_datastore_tables_create_table_column_tpl");

    var nested_columns_tr = $("tr.nested", tpl);
    var nested_columns = $("tr.nested .columns", tpl);

    $("z-dropdown.type", tpl).addEventListener("change", function() {
      if (this.getValue() == "object") {
        nested_columns_tr.classList.remove("hidden");
        nested_columns.innerHTML = "";
        renderColumnRow(nested_columns);
        renderColumnRow(nested_columns);
      } else {
        nested_columns_tr.classList.add("hidden");
      }
    }, false);

    $.onClick($(".add_column", nested_columns_tr), function() {
      renderColumnRow(nested_columns);
    });

    elem.appendChild(tpl);
  };

  return {
    render: render
  }
})();

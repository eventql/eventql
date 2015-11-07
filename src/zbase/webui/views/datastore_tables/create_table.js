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

    $.onClick($("button.submit", tpl), createTable);

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

  var createTable = function() {
    var table_name_input = $(".create_table .table input");
    if (table_name_input.value == 0) {
      $(".create_table .table .error_note.table_name").classList.remove("hidden");
      table_name_input.classList.add("error");
      table_name_input.focus();
      return;
    }

    var json = {
      table_name: table_name_input.value,
      table_type: $(".create_table .table z-dropdown.table_type").getValue(),
      schema: {
        columns: []
      },
      update: false
    };


    json.schema.columns = getColumnsSchema(
      document.querySelectorAll(".create_table .main > .column"));

    //set column ids
    indexColumns(json.schema.columns, 1);
    console.log(JSON.stringify(json));

    $.httpPost("/api/v1/tables/create_table", JSON.stringify(json), function(r) {
      switch (r.status) {
        case 201:
          $.navigateTo("/a/datastore/tables");
          break;

        case 500:
          console.log(r.responseText);
          $(".create_table .error_message .error_text").innerHTML = r.responseText;
          $(".create_table .error_message").classList.remove("hidden");
          break;

        default:
          $.fatalError();
          break;
      }
    });

  };

  var indexColumns = function(columns, id) {
    for (var i = 0; i < columns.length; i++) {
      columns[i].id = id;
      id++;

      if (columns[i].schema && columns[i].schema.columns) {
        id = indexColumns(columns[i].schema.columns, id);
      }
    }

    return id;
  };

  var getColumnsSchema = function(columns) {
    var schema = [];

    for (var i = 0; i < columns.length; i++) {
      var c_input = $(".selection input", columns[i]);
      if (c_input.value == 0) {
        continue;
      }

      var column = {
        name: c_input.value,
        type: $(".selection z-dropdown.type", columns[i]).getValue(),
        optional: $(".selection z-dropdown.is_nullable", columns[i]).getValue()
      };

      if (column.type == "object") {
        column.schema = {columns: getColumnsSchema(
            $(".nested .columns", columns[i]).children)};
      }

      schema.push(column);
    }
    return schema;
  };

  return {
    render: render
  }
})();

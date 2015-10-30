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
        "zbase_datastore_tables_list_tpl");

    var tbody = $("tbody", page);
    tables.forEach(function(table) {
      renderRow(tbody, table);
    });

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), url);

    $.onClick($(".add_pane.create_table", page), displayCreateTableModal);
    var create_modal = $("z-modal.create_table", page);
    $.onClick($("button.submit", create_modal), createTable);
    $.onClick($("button.close", create_modal), function() {
      create_modal.close();
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

  var displayCreateTableModal = function() {
    var modal = $(".zbase_datastore_tables z-modal.create_table");
    var tpl = $.getTemplate(
        "views/datastore_tables",
        "zbase_datastore_tables_add_modal_tpl");

    var column_tpl = $.getTemplate(
        "views/datastore_tables",
        "zbase_datastore_tables_add_modal_column_tpl");

    var columns = $(".columns tbody", tpl);
    columns.appendChild(column_tpl.cloneNode(true));
    columns.appendChild(column_tpl.cloneNode(true));

    $.onClick($(".add_column", tpl), function() {
      columns.appendChild(column_tpl.cloneNode(true));
    });


    $.replaceContent($(".z-modal-content", modal), tpl);
    modal.show();
    $("input", modal).focus();
  };

  var createTable = function() {
    var modal = $(".zbase_datastore_tables z-modal.create_table");
    var input = $("input.table_name", modal);

    if (input.value == 0) {
      $(".error_note.table_name", modal).classList.remove("hidden");
      input.classList.add("error");
      input.focus();
      return;
    }

    var json = {
      table_name: input.value,
      table_type: $("z-dropdown.table_type", modal).getValue(),
      schema: {
        columns: []
      },
      update: false
    };

    var columns = modal.querySelectorAll(".column");
    for (var i = 0; i < columns.length; i++) {
      var c_input = $("input", columns[i]);
      if (c_input.value == 0) {
        continue;
      }

      json.schema.columns.push({
        name: c_input.value,
        type: $("z-dropdown.type", columns[i]).getValue(),
        optional: $("z-dropdown.is_nullable", columns[i]).getValue()
      });
    }

    //set column ids
    for (var i = 0; i < json.schema.columns.length; i++) {
      json.schema.columns[i].id = i + 1;
    }

    $.httpPost("/api/v1/tables/create_table", JSON.stringify(json), function(r) {
      switch (r.status) {
        case 201:
          $.navigateTo("/a/datastore/tables");
          break;

        case 500:
          $(".error_message .error_text", modal).innerHTML = r.responseText;
          $(".error_message", modal).classList.remove("hidden");
          break;

        default:
          $.fatalError();
          break;
      }
    });
  };

  return {
    name: "datastore_tables",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: load
  };
})());

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

    var delete_modal = $("z-modal.delete_table", page);
    $.onClick($("button.submit", delete_modal), function() {
      deleteTable();
    });
    $.onClick($("button.close", delete_modal), function() {
      delete_modal.close();
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

    $("z-dropdown", elem).addEventListener("change", function() {
      if (this.getValue() == "delete") {
        displayDeleteTableModal(table.name);
      }
      this.setValue([]);
    }, false);

    tbody.appendChild(elem);
  };

  var displayCreateTableModal = function() {
    var modal = $(".zbase_datastore_tables z-modal.create_table");
    var tpl = $.getTemplate(
        "views/datastore_tables",
        "zbase_datastore_tables_add_modal_tpl");

    $.replaceContent($(".z-modal-content", modal), tpl);
    modal.show();
    $("input", modal).focus();
  };

  var createTable = function() {
    var input = $(".zbase_datastore_tables z-modal.create_table input");

    if (input.value == 0) {
      $(".zbase_datastore_tables z-modal.create_table .error_note")
          .classList.remove("hidden");
      input.classList.add("error");
      input.focus();
      return;
    }

    var url = "/api/v1/tables/add_table?table=" + input.value;
    $.httpPost(url, "", function(r) {
      if (r.status == 201) {
        var table = JSON.parse(r.response);
        //$.navigateTo("/a/tables/" + table.name);
      } else {
        $.fatalError();
      }
    });
  };

  var deleteTable;
  var onDelete = function(table_name) {
    var url = "/api/v1/tables/delete_table?table=" + table_name;
    $.httpPost(url, "", function(r) {
      if (r.status == 201) {
        console.log(JSON.parse(r.response));
      } else {
        $.fatalError();
      }
    });
  };

  var displayDeleteTableModal = function(table_name) {
    var modal = $(".zbase_datastore_tables z-modal.delete_table");
    $(".table_name", modal).innerHTML = table_name;

    deleteTable = function() {
      onDelete(table_name);
    }
    modal.show();
  };

  return {
    name: "datastore_tables",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: render
  };
})());

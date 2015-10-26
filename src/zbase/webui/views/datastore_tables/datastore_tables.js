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
    var modal = $("z-modal.create_table", page);
    $.onClick($("button.submit", modal), createTable);
    $.onClick($("button.close", modal), function() {
      modal.close();
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
    //$(".table_description", elem).innerHTML = table[1];

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
        var def = JSON.parse(r.response);
        //$.navigateTo("/a/logs/view/" + def.name);
      } else {
        $.fatalError();
      }
    });
  };

  return {
    name: "datastore_tables",
    loadView: function(params) { load(params.path); },
    unloadView: destroy,
    handleNavigationChange: render
  };
})());

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
        "views/table_editor",
        "zbase_table_editor_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), kPathPrefix);

    var table_breadcrumb = $("zbase-breadcrumbs-section.table_name a", page);
    table_breadcrumb.innerHTML = schema.name;
    table_breadcrumb.href = kPathPrefix + schema.name;

    $.onClick($(".add_pane", page), function() {
        displayAddColumnModal(schema.name);
    });

    var delete_modal = $("z-modal.delete_column", page);
    $.onClick($("button.close", delete_modal), function() {
      delete_modal.close();
    });
    $.onClick($("button.submit", delete_modal), function() {
      deleteColumn();
    });

    var tbody = $("tbody", page);
    var row_tpl = $.getTemplate(
        "views/table_editor",
        "zbase_table_editor_row_tpl");

    schema.columns.forEach(function(column) {
      var tr = row_tpl.cloneNode(true);
      $(".name", tr).innerHTML = column.column_name;
      $(".type", tr).innerHTML = column.type;
      $(".is_nullable", tr).innerHTML = column.is_nullable;

      $.onClick($(".delete", tr), function() {
        displayDeleteColumnModal(schema.name, column.column_name);
      });

      tbody.appendChild(tr);
    });

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var displayAddColumnModal = function(table_name) {
    var modal = $(".zbase_table_editor z-modal.add_column");
    var tpl = $.getTemplate(
        "views/table_editor", 
        "zbase_table_editor_add_modal_tpl");

    var input = $("input", tpl);;

    $.onClick($("button.submit", tpl), function() {
      if (input.value.length == 0) {
        $(".error_note", modal).classList.remove("hidden");
        input.classList.add("error");
        input.focus();
        return;
      }

      var url = "/api/v1/tables/add_column?table=" + table_name + "&" +
        $.buildQueryString({
          column_name: input.value,
          type: $("z-dropdown.type", modal).getValue(),
          is_nullable: $("z-dropdown.is_nullable", modal).getValue()});

      //TODO httpPost

    });

    $.onClick($("button.close", tpl), function() {
      modal.close();
    });

    $.replaceContent($(".container", modal), tpl);
    modal.show();
    input.focus();
  };

  var deleteColumn;

  var onDelete = function(table_name, column_name) {
    console.log("delete column ", column_name);
  };

  var displayDeleteColumnModal = function(table_name, column_name) {
    var modal = $(".zbase_table_editor z-modal.delete_column");
    $(".column_name", modal).innerHTML = column_name;

    deleteColumn = function() {
      onDelete(table_name, column_name);
    };

    modal.show();
  };

  return {
    name: "table_editor",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

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

  return {
    name: "table_editor",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

var TableSchemaModalWidget = function(elem) {
  var renderModal = function(table) {
    $(".z-modal-header", modal).innerHTML =
        "Schema for Table: " + $.escapeHTML(table);

    showLoader();
    modal.show();

    $.httpGet("/api/v1/tables/" + table, function(r) {
      if (r.status == 200) {
        var response = JSON.parse(r.response).table;
        var tbody = $("tbody", modal);

        tbody.innerHTML = "";
        response.columns.forEach(function(col) {
          var tr = document.createElement("tr");

          var name_td = document.createElement("td");
          name_td.innerHTML = $.escapeHTML(col.column_name);
          tr.appendChild(name_td);

          var type_td = document.createElement("td");
          type_td.innerHTML = $.escapeHTML(col.type);
          tr.appendChild(type_td);

          var nullable_td = document.createElement("td");
          nullable_td.innerHTML = $.escapeHTML(col.is_nullable);
          tr.appendChild(nullable_td);

          tbody.appendChild(tr);
        });
      } else {
        modal.close();
        $.fatalError();
      }

      hideLoader();
    });
  };

  var showLoader = function() {
    $("table", modal).classList.add("hidden");
    $(".zbase_loader", modal).classList.remove("hidden");
  };

  var hideLoader = function() {
    $("table", modal).classList.remove("hidden");
    $(".zbase_loader", modal).classList.add("hidden");
  };


  var tpl = $.getTemplate(
    "widgets/zbase-table-schema-modal",
    "zbase_table_schema_modal_tpl");
  var modal = $("z-modal", tpl);
  elem.appendChild(tpl);

  return {
    render: renderModal
  }
};

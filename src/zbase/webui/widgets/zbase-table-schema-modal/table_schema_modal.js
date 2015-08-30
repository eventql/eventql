var TableSchemaModalWidget = function(elem) {
  var renderModal = function(table) {
    $(".z-modal-header", modal).innerHTML = table;
    showLoader();
    modal.show();

    $.httpGet("/api/v1/tables/" + table, function(r) {
      if (r.status == 200) {
        var response = JSON.parse(r.response);
        var thead = $("thead tr", modal);
        var tbody = $("tbody", modal);

        thead.innerHTML = "";
        response.columns.forEach(function(column) {
          thead.innerHTML += "<th>" + column + "</th>";
        });

        tbody.innerHTML = "";
        response.rows.forEach(function(row) {
          var tr = document.createElement("tr");
          row.forEach(function(cell) {
            var td = document.createElement("td");
            td.innerHTML = cell;
            tr.appendChild(td);
          });
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

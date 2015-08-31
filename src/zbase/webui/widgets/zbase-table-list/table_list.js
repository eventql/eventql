var TableListWidget = function(elem) {
  var schema_modal = TableSchemaModalWidget(elem);
  var tpl = $.getTemplate(
    "widgets/zbase-table-list",
    "zbase_table_list_tpl");

  var renderTableList = function() {
    elem.appendChild(tpl);

    $.httpGet("/api/v1/tables", function(r) {
      if (r.status == 200) {
        var ul_elem = $(".zbase_table_list ul", elem);
        var tables = JSON.parse(r.response).tables;

        tables.forEach(function(table) {
          var li_elem = document.createElement("li");
          li_elem.innerHTML = table.name;
          ul_elem.appendChild(li_elem);

          $.onClick(li_elem, function() {
            schema_modal.render(table.name);
          });
        });

      } else {
        //TODO handle error
      }

      hideLoader();
    });
  };

  var hideLoader = function() {
    $(".zbase_table_list ul", elem).classList.remove("hidden");
    $(".zbase_table_list .zbase_loader", elem).classList.add("hidden");
  };

  return {
    render: renderTableList
  }
};

var TableListWidget = function(elem) {
  var elem = elem;
  var schema_modal = TableSchemaModalWidget(elem);

  var loadTableList = function() {
    $.httpGet("/api/v1/tables", function(r) {
      if (r.status == 200) {
        renderTableList(JSON.parse(r.response).tables);
      } else {
        //TODO handle error
      }
    });
  };

  var renderTableList = function(tables) {
    var tpl = $.getTemplate(
      "widgets/zbase-table-list",
      "zbase_table_list_tpl");

    var ul_elem = $("ul", tpl);
    tables.forEach(function(table) {
      var li_elem = document.createElement("li");
      li_elem.innerHTML = table.name;
      ul_elem.appendChild(li_elem);

      $.onClick(li_elem, function() {
        schema_modal.render(table.name);
      });
    });

    elem.appendChild(tpl);
    return;
  };

  return {
    render: loadTableList
  }
};

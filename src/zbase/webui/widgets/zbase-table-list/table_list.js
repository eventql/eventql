var TableListWidget = function() {


  var loadTableList = function(elem) {
    $.httpGet("/api/v1/tables", function(r) {
      if (r.status == 200) {
        renderTableList(elem, JSON.parse(r.response).tables);
      } else {
        //TODO handle error
      }
    });

  };

  var renderTableList = function(elem, tables) {
    var tpl = $.getTemplate(
      "widgets/zbase-table-list",
      "zbase_table_list_tpl");

    var ul_elem = $("ul", tpl);
    tables.forEach(function(table) {
      var li_elem = document.createElement("li");
      li_elem.innerHTML = table.name;
      ul_elem.appendChild(li_elem);
    });

    elem.appendChild(tpl);
    return;
  };

  return {
    render: loadTableList,
  }
};

  /*function renderTableDescriptionModal(table) {
    var modal = document.querySelector(".zbase_sql_editor_modal.table_descr");
    var loader = modal.querySelector(".zbase_loader");
    var source_id = "table_descr";
    var source = source_handler.get(
      source_id,
      "/api/v1/sql_stream?query=" + encodeURIComponent("DESCRIBE '" + table + "';"));

    modal.querySelector(".zbase_modal_header").innerHTML = table;
    loader.classList.remove("hidden");
    modal.show();

    source.addEventListener('result', function(e) {
      source_handler.close(source_id);
      var result = JSON.parse(e.data).results[0];
      var header = modal.querySelector("thead tr");
      var body = modal.querySelector("tbody");

      header.innerHTML = "";
      result.columns.forEach(function(column) {
        header.innerHTML += "<th>" + column + "</th";
      });

      body.innerHTML = "";
      result.rows.forEach(function(row) {
        var html = "<tr>"
        row.forEach(function(cell) {
          html += "<td>" + cell + "</td>";
        });
        html += "</tr>";
        body.innerHTML += html;
      });

      loader.classList.add("hidden");
      modal.querySelector(".error_message").classList.add("hidden");
      modal.querySelector(".z-table").classList.remove("hidden");
    });

    source.addEventListener("error", function(e) {
      source_handler.close(source_id);
      var error_message = modal.querySelector(".error_message");
      try {
        error_message.innerHTML = JSON.parse(e.data).error;
      } catch (err) {
        error_message.innerHTML = e.data;
      }

      loader.classList.add("hidden");
      modal.querySelector(".z-table").classList.add("hidden");
      error_message.classList.remove("hidden");
    });
  };*/



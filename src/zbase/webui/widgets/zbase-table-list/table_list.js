
            <h3 class="sidebar_title">
              <i class="fa fa-database"></i>&nbsp;Tables
            </h3>

            <ul class="zbase_sql_editor_table_list" style="display:none;"></ul>

  function renderTableList(pane) {
    var ul = pane.querySelector(".zbase_sql_editor_table_list");
    var source_id = "tables";
    var source = source_handler.get(
      source_id,
      "/api/v1/sql_stream?query=" + encodeURIComponent("SHOW TABLES;"));

    source.addEventListener('result', function(e) {
      source_handler.close(source_id);

      var data = JSON.parse(e.data);
      data.results[0].rows.forEach(function(table) {
        var li = document.createElement("li");
        li.innerHTML = table[0];
        ul.appendChild(li);
        li.addEventListener('click', function() {
          renderTableDescriptionModal(table[0]);
        }, false);
      });
      pane.querySelector(".sidebar_section .zbase_loader").classList.add("hidden");
      ul.style.display = "block";
    });
  };

  function renderTableDescriptionModal(table) {
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
  };


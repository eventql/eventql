var LogfileTable = function(rows) {

  var render = function(elem) {
    var tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_table_tpl");

    var tbody = $("table tbody", tpl);
    for (var i = 0; i < rows.length; i++) {
      renderRow(tbody, rows[i]);
    }

    $.replaceContent(elem, tpl);
  }

  // FIXME combine the three renderRow methods below into 1 or 2 methods

  var renderRow = function(tbody, row) {
    if (row.columns.length > 0) {
      if (row.raw) {
        renderStructuredWithRawRow(tbody, row);
      } else {
        renderStructuredRow(tbody, row);
      }
    } else {
      renderRawRow(tbody, row);
    }
  }

  var renderRawRow = function(tbody, row) {
    var tr = document.createElement('tr');
    tr.className = "folding";

    tr.innerHTML =
        "<td class='time'>" +
        DateUtil.printTimestamp(row.time / 1000) +
        "</td><td><span>" + $.wrapText(row.raw) + "</span></td>";


    tbody.appendChild(tr);
  };

  var renderStructuredWithRawRow = function(tbody, row) {
    var tr = document.createElement('tr');
    var folded_tr = document.createElement("tr");
    tr.className = "folding";

    tr.innerHTML =
      "<td class='fold_icon'><i class='fa'></i></td><td class='time'>" +
      DateUtil.printTimestamp(row.time / 1000) +"</td>";

    row.columns.forEach(function(column) {
      tr.innerHTML += "<td><span>" + $.wrapText(column) + "</span></td>";
    });

    folded_tr.className = "folded";
    folded_tr.innerHTML = 
      "<td colspan='" + (row.columns.length + 2) + "'><span>" +
      $.wrapText(row.raw) + "</span></td>";
    tbody.appendChild(tr);
    tbody.appendChild(folded_tr);

    tr.addEventListener("click", function() {
      if (this.hasAttribute("data-active")) {
        this.removeAttribute("data-active");
      } else {
        this.setAttribute("data-active", "active");
      }
    }, false);
  };

  this.renderStructuredRow = function(tbody, row) {
    var tr = document.createElement('tr');
    tr.innerHTML =
        "<td class='time'>" + DateUtil.printTimestamp(row.time / 1000) +"</td>";

    row.columns.forEach(function(column) {
      tr.innerHTML += "<td><span>" + $.wrapText(column) + "</span></td>";
    });

    tbody.appendChild(tr);
  };

  return {
    render: render
  };

};

var PerSellerTableOverview = (function() {
  var render = function(elem, result) {
    var tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_main_tpl");

    var tbody = $("tbody", tpl);
    result.rows.forEach(function(row) {
      var tr = document.createElement("tr");

      for (var i = 1; i < row.length; i++) {
        var td = document.createElement("td");
        td.innerHTML = row[i];
        tr.appendChild(td);
      }

      tbody.appendChild(tr);
    });

    $.replaceContent(elem, tpl);
  };

  return {
    render: render
  }
})();

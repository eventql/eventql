var PerSellerTableOverview = (function() {
  var render = function(elem, result) {
    var tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_main_tpl");

    var tr_tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_row_tpl");

    var tbody = $("tbody", tpl);

    //renderHEADER

    for (var i = 0; i < result.timeseries.time.length; i++) {
      var tr = tr_tpl.cloneNode(true);

      for (var metric in result.timeseries) {
        var td = $("td." + metric, tr);

        if (td) {
          td.innerHTML = result.timeseries[metric][i];
          tr.appendChild(td);
        }
      }

      tbody.appendChild(tr);
    }

    $.replaceContent(elem, tpl);
  };

  return {
    render: render
  }
})();

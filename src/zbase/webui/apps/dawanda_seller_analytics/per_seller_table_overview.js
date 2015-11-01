var PerSellerTableOverview = (function() {
  var render = function(elem, result, metrics) {
    var tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_main_tpl");

    var table = $("table", tpl);
    $.replaceContent(elem, tpl);

    renderTable(table, result.timeseries, metrics);
  };

  var renderTable = function(elem, timeseries, metrics) {
    var table_tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_table_tpl");

    var tr_tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_row_tpl");

    var tbody = $("tbody", table_tpl);

    for (var metric in metrics) {
      $("th." + metric, table_tpl).classList.remove("hidden");
    }

    for (var i = 0; i < timeseries.time.length; i++) {
      var tr = tr_tpl.cloneNode(true);

      for (var metric in timeseries) {
        var td = $("td." + metric, tr);

        //REMOVEME 
        if (!td) {
          continue;
        }
        //REMOVEME END

        if (metrics.hasOwnProperty(metric)) {
          td.classList.remove("hidden");
          td.innerHTML = ZBaseSellerMetrics[metric].print(timeseries[metric][i]);
        }
      }

      tbody.appendChild(tr);
    }

    $.replaceContent(elem, table_tpl);
  };

  return {
    render: render
  }
})();

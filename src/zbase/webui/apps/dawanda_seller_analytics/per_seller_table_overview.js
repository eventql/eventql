var PerSellerTableOverview = (function() {
  var render = function(elem, result, metrics) {
    var tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_main_tpl");

    var table = $("table", tpl);
    $.replaceContent(elem, tpl);

    renderTable(table, result, metrics);
  };

  var renderTable = function(elem, result, metrics) {
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

    var aggr_tr = tr_tpl.cloneNode(true);
    for (var metric in result.aggregates) {
      if (metrics.hasOwnProperty(metric)) {
        var td = $("td." + metric, aggr_tr);

        //REMOVEME 
        if (!td) {
          console.log("no td");
          continue;
        }
        //REMOVEME END

        td.classList.remove("hidden");
        td.innerHTML = result.aggregates[metric];
      }
    }
    $(".time", aggr_tr).innerHTML = "Total";
    tbody.appendChild(aggr_tr);

    for (var i = 0; i < result.timeseries.time.length; i++) {
      var tr = tr_tpl.cloneNode(true);

      for (var metric in result.timeseries) {
        var td = $("td." + metric, tr);

        //REMOVEME 
        if (!td) {
          continue;
        }
        //REMOVEME END

        if (metrics.hasOwnProperty(metric)) {
          td.classList.remove("hidden");
          td.innerHTML = ZBaseSellerMetrics[metric].print(
              result.timeseries[metric][i]);
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

var PerSellerTableOverview = (function() {
  var render = function(elem, result, path) {
    var tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_main_tpl");

    setParamTableMetrics(UrlUtil.getParamValue("table_metrics", path));

    $("z-dropdown.metrics", tpl).addEventListener("change", function() {
      console.log("render table");
      //set param table metrics
    }, false);

    $.replaceContent(elem, tpl);

    renderTable(result.timeseries);
  };

  var renderTable = function(timeseries) {
    var tbody = $(".per_seller_table tbody");
    var tr_tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_row_tpl");

    var metrics = {};
    getParamTableMetrics().split(",").forEach(function(metric) {
      metrics[metric] = true;
    });
    console.log(metrics);

    for (var i = 0; i < timeseries.time.length; i++) {
      var tr = tr_tpl.cloneNode(true);

      for (var metric in timeseries) {
        var td = $("td." + metric, tr);

        //REMOVEME
        if (!td) {
          continue;
        }
        //REMOVEME END

        if (metric == "time" || metrics.hasOwnProperty(metric)) {
          td.classList.remove("hidden");
          td.innerHTML = timeseries[metric][i];
        }
      }

      tbody.appendChild(tr);
    }

  };

  var setParamTableMetrics = function(metrics) {
    if (metrics) {
      $(".per_seller_table z-dropdown.metrics").setValue(metrics.split(","));
    }
  };

  var getParamTableMetrics = function() {
    return $(".per_seller_table z-dropdown.metrics").getValue();
  };

  return {
    render: render
  }
})();

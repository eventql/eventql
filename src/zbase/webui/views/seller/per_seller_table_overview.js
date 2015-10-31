var PerSellerTableOverview = (function() {
  var render = function(elem, result, path) {
    var tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_main_tpl");

    var table = $("table", tpl);
    $("z-dropdown.metrics", tpl).addEventListener("change", function() {
      renderTable(table, result.timeseries);
      var new_path = UrlUtil.addOrModifyUrlParam(
          path,
          "metrics",
          encodeURIComponent(this.getValue()));
      history.pushState({path: new_path}, "", new_path);
    }, false);

    $.replaceContent(elem, tpl);

    setParamTableMetrics(UrlUtil.getParamValue(path, "metrics"));
    renderTable(table, result.timeseries);
  };

  var renderTable = function(elem, timeseries) {
    var table_tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_table_tpl");

    var tr_tpl = $.getTemplate(
        "views/seller",
        "per_seller_table_overview_row_tpl");

    var tbody = $("tbody", table_tpl);

    var metrics = {time: true};
    getParamTableMetrics().split(",").forEach(function(metric) {
      metrics[metric] = true;
    });

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
          td.innerHTML = timeseries[metric][i];
        }
      }

      tbody.appendChild(tr);
    }

    $.replaceContent(elem, table_tpl);
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

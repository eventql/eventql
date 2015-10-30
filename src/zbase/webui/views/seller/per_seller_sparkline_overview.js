var PerSellerTableOverview = (function() {
  var render = function(elem, result) {
    var tpl = $.getTemplate(
      "views/seller",
      "per_seller_sparkline_overview_tpl");

    var metric_values = [];

    console.log(result);

    for (var i = 1; i < result.rows[0].length; i++) {
      metric_values.push([]);
    };

    for (var i = 0; i < result.rows.length; i++) {
      for (var j = 1; j < result.rows[i].length; j++) {
        metric_values[j-1].push(result.rows[i][j]);
      }
    }


    var metric_panes = tpl.querySelectorAll(".metric_pane");

    $.replaceContent(elem, tpl);

    console.log(metric_panes.length, metric_values.length);

    for (var i = 0; i < metric_panes.length; i++) {
      //$(".num", pane).innerHTML = data.aggregates[metric];
      $("z-sparkline", metric_panes[i]).setAttribute(
          "data-sparkline",
          metric_values[i].join(","));

    }
  };


  return {
    render: render
  }
})();

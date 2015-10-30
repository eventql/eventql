var PerSellerSparklineOverview = (function() {
  var render = function(elem, result) {
    var tpl = $.getTemplate(
      "views/seller",
      "per_seller_sparkline_overview_tpl");

    $.replaceContent(elem, tpl);

    for (var metric in result.timeseries) {
      var pane = $(".zbase_seller_stats .metric_pane." + metric);

      if (pane) {
        $("z-sparkline", pane).setAttribute(
            "data-sparkline",
            result.timeseries[metric].join(","));
      }
    }

      //$(".num", metric_panes[i]).innerHTML = sum;
  };

  var add = function(a, b) {
    return parseFloat(a) + parseFloat(b);
  };

  var toPrecision = function(num, precision) {
    return Math.round(num * Math.pow(10, precision)) / Math.pow(10, precision);
  };


  return {
    render: render
  }
})();

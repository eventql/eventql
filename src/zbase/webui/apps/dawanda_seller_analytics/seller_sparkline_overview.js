var SellerSparklineOverview = (function() {
  var render = function(elem, result) {
    var tpl = $.getTemplate(
      "views/seller",
      "per_seller_sparkline_overview_tpl");

    $.replaceContent(elem, tpl);

    var start = DateUtil.printDate(Math.round(
        result.timeseries.time[0] / 1000));
    var end = DateUtil.printDate(Math.round(
        result.timeseries.time[result.timeseries.time.length - 1] / 1000));

    for (var metric in result.timeseries) {
      var pane = $(".zbase_seller_stats .metric_pane." + metric);

      if (pane) {
        $("z-chart", pane).render(
            result.timeseries.time,
            result.timeseries[metric]
              .map(function(v) {
                return parseFloat(v);
              }));

        $(".num", pane).innerHTML = result.aggregates[metric];
        $(".start", pane).innerHTML = start;
        $(".end", pane).innerHTML = end;

        $(".zbase_seller_stats z-tooltip." + metric).init($(".help", pane))
      }
    }

    //REMOVE ME
    $(".zbase_seller_stats z-chart").render(
        ["1.10", "2.10", "3.10", "4.10"],
        [1, 2, 3, 4]);
    //REMOVE ME END
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

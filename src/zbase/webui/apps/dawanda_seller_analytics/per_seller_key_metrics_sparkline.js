var PerSellerKeyMetricsSparkline = (function() {
  var render = function(elem, result) {
    var tpl = $.getTemplate(
        "apps/dawanda_seller_analytics",
        "per_seller_key_metrics_sparkline_main_tpl");

    $.replaceContent(elem, tpl);

    var start = DateUtil.printDate(Math.round(
        result.timeseries.time[0] / 1000));
    var end = DateUtil.printDate(Math.round(
        result.timeseries.time[result.timeseries.time.length - 1] / 1000));

    for (var metric in result.timeseries) {
      var pane = $(".zbase_seller_stats .metric_pane." + metric);

      if (pane) {
        var z_chart = $("z-chart", pane);
        z_chart.render(
            result.timeseries.time,
            result.timeseries[metric]
              .map(function(v) {
                return parseFloat(v);
              }));

        z_chart.formatX = function(value) {
          return ZBaseSellerMetrics.time.print(value);
        };

        z_chart.formatY = formatY(metric);

        $(".num", pane).innerHTML = result.aggregates[metric];
        $(".start", pane).innerHTML = start;
        $(".end", pane).innerHTML = end;

        $(".zbase_seller_stats z-tooltip." + metric).init($(".help", pane))
      }
    }
  };

  var formatY = function(metric) {
    return function(value) {
      return ZBaseSellerMetrics[metric].print(value);
    }
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

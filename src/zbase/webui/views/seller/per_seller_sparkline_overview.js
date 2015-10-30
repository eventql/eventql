var PerSellerSparklineOverview = (function() {
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


    for (var i = 0; i < metric_values.length; i++) {
      //$(".num", metric_panes[i]).innerHTML = sum;

      $("z-sparkline", metric_panes[i]).setAttribute(
          "data-sparkline",
          metric_values[i].join(","));

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

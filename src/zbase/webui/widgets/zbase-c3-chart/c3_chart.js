ZBaseC3Chart = function(config) {
  var chart = null;

  var destroy = function() {
    window.removeEventListener("resize", resize, false);
  };

  var renderLegend = function(elem) {
    elem.innerHTML = "";

    var tpl = $.getTemplate(
        "widgets/zbase-c3-chart",
        "zbase-c3-chart-legend-item-tpl");

    for (var i = 0; i < config.data.y.length; i++) {
      var html = tpl.cloneNode(true);
      $(".circle", html).style.backgroundColor = config.data.y[i].color;
      $(".legend_item_name", html).innerHTML = config.data.y[i].name;
      elem.appendChild(html);
    }
  };

  var renderTimeseries = function(elem_id) {
    var x_config = {
      type: "timeseries"
    };

    render(x_config, elem_id);
  };

  var render = function(x_pre_config, elem_id) {
    var y_config = {
      min: 0,
      label: {
        position: "inner-middle"
      },
      padding: {
        top: 50,
        bottom: 0
      }
    };

    var x_config = mergeObjects(
        {tick: {count: 10, format: config.format}},
        x_pre_config);

    config.data.x.unshift("x");
    var columns = [config.data.x];
    var colors = {};
    config.data.y.forEach(function(y_set) {
      y_set.values.unshift(y_set.name);
      columns.push(y_set.values);
      colors[y_set.name] = y_set.color;
    });

    chart = c3.generate({
      bindto: "#" + elem_id,
      interaction: true,
      data: {
        x: 'x',
        type: config.type,
        columns: columns,
        selection: {
          enabled: true
        },
        colors: config.colors
      },
      axis: {
        x: x_config,
        y: y_config
      },
      line: {
        connectNull: true
      },
      grid: {
        y: {
          show: true
        }
      },
      point: {
        show: false,
        r: 3.5
      },
      legend: {
        show: false
      }
    });
  };

  //Overwrites x'values with y'values and adds y's values if not existent in x
  var mergeObjects = function(x, y) {
    var z = {};
    for (var attr in x) {
      z[attr] = x[attr];
    }

    for (var attr in y) {
      if (typeof z[attr] == "object" && typeof y[attr] == "object") {
        z[attr] = mergeObjects(z[attr], y[attr]);
        continue;
      }
      z[attr] = y[attr];
    }
    return z;
  };

  var resize = function() {
    if (chart != null) {
      chart.resize();
    }
  };

  window.addEventListener("resize", resize, false);


  return {
    renderLegend: renderLegend,
    renderTimeseries: renderTimeseries,
    destroy: destroy
  }
};

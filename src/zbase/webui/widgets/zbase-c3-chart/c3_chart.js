ZBaseC3Chart = function(config) {
  var chart = null;

  var destroy = function() {
    window.removeEventListener("resize", resize, false);
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

    chart = c3.generate({
      bindto: "#" + elem_id,
      interaction: true,
      data: {
        x: 'x',
        type: 'line',
        columns: [
          config.data.x,
          config.data.y
        ],
        selection: {
          enabled: true
        },
        colors: config.colors
        //colors names columns
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
        show: true,
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
    renderTimeseries: renderTimeseries,
    destroy: destroy
  }
};

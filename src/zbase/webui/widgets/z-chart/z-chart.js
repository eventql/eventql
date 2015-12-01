/**
  This file is part of the "FnordMetric" project
    Copyright (c) 2015 Laura Schlimmer
    Copyright (c) 2015 Paul Asmuth

  FnordMetric is free software: you can redistribute it and/or modify it under
  the terms of the GNU General Public License v3.0. You should have received a
  copy of the GNU General Public License along with this program. If not, see
  <http://www.gnu.org/licenses/>.
*/

var ZChartComponent = function() {
  // renders line chart
  this.render = function(x_values, y_values) {
    var data = {
      x: x_values,
      y: [{
          values: y_values,
          min: (this.hasAttribute('data-min')) ?
            parseFloat(this.getAttribute('data-min')) : 0.0,
          max: (this.hasAttribute('data-max')) ?
            parseFloat(this.getAttribute('data-max')) : Math.max.apply(null, y_values)
      }]
    };

    this.renderChart(data);
  };

  //render multiple lines
  this.renderLines = function(values) {
    this.renderChart(values);
  };

  this.renderChart = function(data) {
    var height = this.getDimension('height');
    var width = this.getDimension("width");

    var tpl = $.getTemplate(
        "widgets/z-chart",
        "z-chart-tpl");

    this.innerHTML = "";
    this.appendChild(tpl);

    var svg = this.querySelector("svg");
    svg.style.height = height + "px";
    svg.style.width = width + "px";

    for (var i = 0; i < data.y.length; i++) {
      if (!data.y[i].min) {
        data.y[i].min = 0;
      }
      if (!data.y[i].max) {
        data.y[i].max = Math.max.apply(null, data.y[i].values);
      }
      this.renderLine(height, width, data.x, data.y[i]);
    }
  };

  this.renderLine = function(height, width, data_x, data_y) {
    var points = this.scaleValues(data_y);
    var svg = this.querySelector("svg");
    var padding_x = 8;
    var padding_y = 8;

    var svg_line = [];
    var svg_circles = [];
    for (var i = 0; i < points.length; ++i) {
      if (!isNaN(points[i].y)) {
        var dx = padding_x + (points[i].x * (width - padding_x * 2));
        var dy = padding_y + ((1.0 - points[i].y) * (height - padding_y * 2));
        svg_line.push(i == 0 ? "M" : "L", dx, dy);
        svg_circles.push([dx, dy]);
      }
    }

    //var path = this.querySelector("path");
    var path = document.createElement("path");
    svg.appendChild(path);
    path.setAttribute("d", svg_line.join(" "));

    var rect_width = width / (data_x.length - 1);
    var x = (rect_width / 2) * -1;

    for (var i = 0; i < svg_circles.length; i++) {
      svg.innerHTML += "<g><circle cx='" + svg_circles[i][0] + "' cy='" +
          svg_circles[i][1] + "' r='2.5' />" +
          "<rect width='" + rect_width  + "' height='" + height +
          "' y='0' x='" + x + "' /></g>";
      x += rect_width;
    }

    var groups = svg.querySelectorAll("g");
    for (var i = 0; i < groups.length; i++) {
      this.setupTooltip(groups[i], data_x[i], data_y.values[i]);
    }
  }




  this.setupTooltip = function(html, x_value, y_value) {
    var tooltip = this.querySelector("z-chart-tooltip");
    var rect = html.querySelector("rect");
    var _this = this;
    rect.addEventListener("mouseenter", function(e) {
      tooltip.innerHTML = _this.formatX(x_value) + ": " + _this.formatY(y_value);
      tooltip.classList.remove("hidden");

      var pos = html.querySelector("circle").getBoundingClientRect();
      tooltip.style.top = (pos.top - pos.height - tooltip.offsetHeight) + "px";
      tooltip.style.left = (pos.left - tooltip.offsetWidth / 2) + "px";

    }, false);

    tooltip.addEventListener("mouseenter", function(e) {
      this.classList.remove("hidden");
    }, false);

    tooltip.addEventListener("mouseout", function(e) {
      this.classList.add("hidden");
    }, false);

    rect.addEventListener("mouseout", function(e) {
      tooltip.classList.add("hidden");
    }, false);
  };

  this.getDimension = function(dimension) {
    var value = this.getAttribute(dimension);

    var idx = value.indexOf("%");
    if (idx > -1) {
      return this.offsetWidth * (parseInt(value.substr(0, idx), 10) / 100);
    }

    return parseFloat(value);
  };

  this.scaleValues = function(y) {
    var scaled = [];

    for (var i = 0; i < y.values.length; ++i) {
      var v = y.values[i];
      var x  = i / (y.values.length - 1);

      if (v < y.min) {
        scaled.push({x : x, y: 0});
        continue;
      }

      if (v > y.max) {
        scaled.push({x: x, y: 1.0});
        continue;
      }

      if (y.max - y.min == 0) {
        scaled.push({x: x, y: v});
      } else {
        scaled.push({x: x, y:(v - y.min) / (y.max - y.min)});
      }
    }

    return scaled;
  };


};

var proto = Object.create(HTMLElement.prototype);
ZChartComponent.apply(proto);
document.registerElement("z-chart", { prototype: proto });

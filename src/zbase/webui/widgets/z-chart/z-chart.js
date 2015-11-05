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
  this.createdCallback = function() {
    console.log("z chart component");
  };

  this.render = function(x_values, y_values) {
    var y = {};
    y.values = y_values;

    y.min = (this.hasAttribute('data-min')) ?
      parseFloat(this.getAttribute('data-min')) : 0.0;
    y.max = (this.hasAttribute('data-max')) ?
      parseFloat(this.getattribute('data-max')) : Math.max.apply(null, y_values);

    this.renderChart(y);
  };

  this.renderChart = function(y) {
    var height = this.getDimension('height');
    var width = this.getDimension("width");
    var padding_x = 0;
    var padding_y = 5;

    var points = this.scaleValues(y);

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

    console.log(svg_circles);

    var tpl = $.getTemplate(
        "widgets/z-sparkline",
        "z-sparkline-tpl");

    this.innerHTML = "";
    this.appendChild(tpl);

    var svg = this.querySelector("svg");
    svg.style.height = height + "px";
    svg.style.width = width + "px";

    var path = this.querySelector("path");
    path.setAttribute("d", svg_line.join(" "));

    svg_circles.forEach(function(c) {
      svg.innerHTML += "<circle cx='" + c[0] + "' cy='" + c[1] + "' r='2' />";
    });
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

/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */

EventQL.ChartPlotter = function(elem, params) {
  'use strict';

  const RENDER_SCALE_FACTOR = 1.6;

  var width;
  var width_real;
  var height;
  var height_real;
  var canvas_margin_top;
  var canvas_margin_right;
  var canvas_margin_bottom;
  var canvas_margin_left;
  var x_domain;
  var x_ticks_count;
  var x_labels = [];
  var y_ticks_count;
  var y_domain;
  var y_label_width;
  var y_labels = [];

  this.render = function(result) {
    /* prepare data */
    var x_values = [];
    var y_values = [];
    result.rows.forEach(function(row) {
      x_values.push(row[0]);
      y_values.push(parseFloat(row[1]));
    });

    /* prepare axes */
    prepareXAxis(x_values);
    prepareYAxis(y_values);

    /* prepare layout */
    prepareLayout(result);

    /* draw the svg */
    draw(x_values, y_values);

    /* adjust the svg when the elem is resized */
    if (!params.width) {
      //var resize_observer = new ResizeObserver(function(e) {
      //  prepareLayout(result);
      //  draw(result);
      //});

      //resize_observer.observe(elem);
    }
  }

  function prepareLayout(result) {
    width_real = params.width || elem.offsetWidth;
    width = width_real * RENDER_SCALE_FACTOR;
    height_real = params.height || elem.offsetHeight;
    height = height_real * RENDER_SCALE_FACTOR;

    canvas_margin_top = 6 * RENDER_SCALE_FACTOR;
    canvas_margin_right = 1 * RENDER_SCALE_FACTOR;
    canvas_margin_bottom = 18 * RENDER_SCALE_FACTOR;
    canvas_margin_left = 1 * RENDER_SCALE_FACTOR;

    /* fit the y axis labels */
    y_label_width =
        7 * RENDER_SCALE_FACTOR *
        Math.max.apply(null, y_labels.map(function(l) { return l.toString().length; }));

    /* fit the y axis */
    if (params.axis_y_position == "inside") {

    } else {
      canvas_margin_left = y_label_width;
    }
  }

  function prepareXAxis(values) {
    x_ticks_count = 12;
    x_domain = new EventQL.ChartPlotter.LinearDomain;
    x_domain.findMinMax(values);

    for (var i = 0; i <= x_ticks_count ; i++) {
      var label = x_domain.convertScreenToDomain(i / x_ticks_count);
      if (params.timeseries) {
        label = Formatter.formatDate(label);
      }

      x_labels.push(label);
    }
  }

  function prepareYAxis(values) {
    y_ticks_count = 5;
    y_domain = new EventQL.ChartPlotter.LinearDomain;
    y_domain.findMinMax(values);

    /* set up y axis labels */
    for (var i = 0; i <= y_ticks_count ; i++) {
      y_labels.push(y_domain.convertScreenToDomain(1.0 - (i / y_ticks_count)));
    }
  }

  function draw(x_values, y_values) {
    var svg = drawChart(x_values, y_values);
    elem.innerHTML = svg;
  }

  function drawChart(x_values, y_values) {
    var svg = new EventQL.SVGHelper();
    svg.svg += "<svg shape-rendering='geometricPrecision' class='evql-chart' viewBox='0 0 " + width + " " + height + "' style='width:" + width_real + "px;'>";

    drawBorders(svg);
    drawXAxis(svg);
    drawYAxis(svg);

    switch (params.type) {
      case "bar":
        drawBars(x_values, y_values, svg);
        break;

      case "line":
      default:
        drawLine(x_values, y_values, svg);
        if (params.points) {
          drawPoints(x_values, y_values, svg);
        }
        break;
    }

    svg.svg += "</svg>"
    return svg.svg;
  }

  function drawBorders(c) {
    /** render top border **/
    if (params.border_top) {
      c.drawLine(
          canvas_margin_left,
          width - canvas_margin_right,
          canvas_margin_top,
          canvas_margin_top,
          "border");
    }

    /** render right border  **/
    if (params.border_right) {
      c.drawLine(
          width - canvas_margin_right,
          width - canvas_margin_right,
          canvas_margin_top,
          height - canvas_margin_bottom,
          "border");
    }

    /** render bottom border  **/
    if (params.border_bottom) {
      c.drawLine(
          canvas_margin_left,
          width - canvas_margin_right,
          height - canvas_margin_bottom,
          height - canvas_margin_bottom,
          "border");
    }

    /** render left border **/
    if (params.border_left) {
      c.drawLine(
          canvas_margin_left,
          canvas_margin_left,
          canvas_margin_top,
          height - canvas_margin_bottom,
          "border");
    }
  }

  function drawXAxis(c) {
    c.svg += "<g class='axis x'>";

    /** render tick/grid **/
    var text_padding = 6 * RENDER_SCALE_FACTOR;
    for (var i = 1; i < x_ticks_count; i++) {
      var tick_x_domain = (i / x_ticks_count);
      var tick_x_screen = tick_x_domain * (width - (canvas_margin_left + canvas_margin_right)) + canvas_margin_left;

      c.drawLine(
          tick_x_screen,
          tick_x_screen,
          canvas_margin_top,
          height - canvas_margin_bottom,
          "grid");

        var text_padding = 2 * RENDER_SCALE_FACTOR;
        c.drawText(
            tick_x_screen,
            (height - canvas_margin_bottom) + text_padding,
            x_labels[i]);
    }

    c.svg += "</g>";
  }

   function drawYAxis(c) {
    c.svg += "<g class='axis y'>";

    /** render tick/grid **/
    for (var i = 0; i <= y_ticks_count ; i++) {
      var tick_y_domain = (i / y_ticks_count);
      var tick_y_screen = tick_y_domain * (height - (canvas_margin_bottom + canvas_margin_top)) + canvas_margin_top;

      c.drawLine(
          canvas_margin_left,
          width - canvas_margin_right,
          tick_y_screen,
          tick_y_screen,
          "grid");

      if (params.axis_y_position == "inside" && (i == y_ticks_count)) {
        /* skip text */
      } else if (params.axis_y_position == "inside") {
        var text_padding = 2 * RENDER_SCALE_FACTOR;
        c.drawText(
            canvas_margin_left + text_padding,
            tick_y_screen,
            y_labels[i],
            "inside");
      } else {
        var text_padding = 5 * RENDER_SCALE_FACTOR;
        c.drawText(
            canvas_margin_left - text_padding,
            tick_y_screen,
            y_labels[i],
            "outside");
      }
    }

    c.svg += "</g>";
   }

  function drawLine(x_values, y_values, c, opts) {
    var points = [];

    for (var i = 0; i < x_values.length; i++) {
      var x = x_domain.convertDomainToScreen(x_values[i]);
      var x_screen = x * (width - (canvas_margin_left + canvas_margin_right)) + canvas_margin_left;

      if (y_values[i] === null) {
        points.push([x_screen, null]);
      } else {
        var y = y_domain.convertDomainToScreen(y_values[i]);
        var y_screen = (1.0 - y) * (height - (canvas_margin_bottom + canvas_margin_top)) + canvas_margin_top;
        points.push([x_screen, y_screen]);
      }
    }

    c.drawPath(points, "line");
  }

  function drawPoints(x_values, y_values, c) {
    var point_size = 3;

    for (var i = 0; i < x_values.length; i++) {
      var x = x_domain.convertDomainToScreen(x_values[i]);
      var y = y_domain.convertDomainToScreen(y_values[i]);

      var x_screen = x * (width - (canvas_margin_left + canvas_margin_right)) + canvas_margin_left;
      var y_screen = height - (y * (height - (canvas_margin_bottom + canvas_margin_top)) + canvas_margin_bottom);

      c.drawPoint(x_screen, y_screen, point_size, "point");
    }
  }

  function drawBars(x_values, y_values, svg) {
    var bar_padding = 20;
    var bar_width = (width - (canvas_margin_left + canvas_margin_right) - (bar_padding * (y_values.length + 1))) / y_values.length;

    for (var i = 0; i < y_values.length; i++) {
      var y = y_domain.convertDomainToScreen(y_values[i]);
      var bar_height = y * (height - (canvas_margin_bottom + canvas_margin_top)) + canvas_margin_bottom;
      var y_screen = height - (y * (height - (canvas_margin_bottom + canvas_margin_top)) + canvas_margin_bottom);

      var x_screen = canvas_margin_left + i * bar_width + (i + 1) * bar_padding;

      svg.drawRect(x_screen, y_screen, bar_width, bar_height, "bar");
    }

  }

};


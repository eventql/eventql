/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
 * Authors:
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

EventQL.SQLEditor.ChartBuilder = function(elem) {
  'use strict';

  this.render = function(result, type) {
    var chart_elem = document.createElement("div");
    chart_elem.classList.add("chart_container");
    elem.appendChild(chart_elem);

    var chart_cfg = {
      height: 300
    };

    switch (type) {
      case "bar":
        break;

      case "line":
        chart_cfg.timeseries = true; //FIXME make this configurable
        chart_cfg.points = true;
        break;

      case "area":
        chart_cfg.timeseries = true; //FIXME make this configurable
        break;

      default:
        console.log("ERROR unknown chart type", type);
        return;
    }

    chart_cfg.type = type;

    var chart = new EventQL.ChartPlotter(chart_elem, chart_cfg);
    chart.render(result);
  };

};


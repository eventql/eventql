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

EventQL.SQLEditor.ResultList = function(elem, params) {
  'use strict';

  const CHART_COLUMN_NAME = "__chart";

  var on_csv_download = [];

  this.render = function(results) {
    for (var i = 0; i < results.length; i++) {
      if (results[i].columns.length == 0) {
        continue;
      }

      /* render result chart */
      if (results[i].columns[0] == CHART_COLUMN_NAME) {
        if (results[i].rows.length == 0) { //error: no svg returned
          continue;
        }

        renderChart(results[i].rows[0][0], i);

      /* render result table / chart builder */
      } else {
        renderTable(results[i], i);

      }
    }
  };

  this.onCSVDownload = function(fn) {
    on_csv_download.push(fn);
  };

  function renderChart(svg, idx) {
    var result_elem = document.createElement("div");
    elem.appendChild(result_elem);

    var toolbar = new EventQL.SQLEditor.ResultToolbar(result_elem, {
      enable_hide: true,
    });

    toolbar.render(idx);

    var chart_elem = document.createElement("div");
    chart_elem.innerHTML = svg;
    elem.appendChild(chart_elem);
  }

  function renderTable(result, idx) {
    var result_elem = document.createElement("div");
    var data_elem = document.createElement("div");

    function switchDataVisualization(value) {
      DOMUtil.clearChildren(data_elem);

      switch (value) {
        case "table":
          var table = new EventQL.SQLEditor.Table(data_elem);
          table.render(result);
          break;

        case "bar":
        case "line":
          var chart_builder = new EventQL.SQLEditor.ChartBuilder(data_elem);
          chart_builder.render(result, value);
          break;
      }
    }

    var toolbar = new EventQL.SQLEditor.ResultToolbar(result_elem, {
      enable_hide: true,
      has_controls: true
    });

    toolbar.onCSVDownload(function() {
      on_csv_download.forEach(function(callback_fn) {
        callback_fn(result);
      });
    });

    toolbar.render(idx);
    toolbar.onDataVisualizationChange(switchDataVisualization);

    switchDataVisualization(toolbar.getDataVisualization());

    elem.appendChild(result_elem);
    result_elem.appendChild(data_elem);
  }

};


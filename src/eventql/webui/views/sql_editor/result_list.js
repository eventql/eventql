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
  var on_params_change = [];

  this.onCSVDownload = function(fn) {
    on_csv_download.push(fn);
  };

  this.onParamsChange = function(fn) {
    on_params_change.push(fn);
  };

  var is_hidable;

  this.render = function(results) {
    is_hidable = results.length > 1;

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

      } else {
        renderTable(results[i], i);
      }
    }
  };

  function renderChart(svg, idx) {
    var result_elem = document.createElement("div");
    elem.appendChild(result_elem);

    var chart_cfg = {
      hidable: is_hidable,
      idx: idx
    };
    var chart = new EventQL.SQLEditor.ResultList.Chart(result_elem, chart_cfg);
    chart.render(svg);
  }

  function renderTable(result, idx) {
    var result_elem = document.createElement("div");
    elem.appendChild(result_elem);

    var table_chart_cfg = {
      hidable: is_hidable,
      idx: idx
    };

    var table_chart = new EventQL.SQLEditor.ResultList.TableChartBuilder(
        result_elem,
        table_chart_cfg);
    table_chart.render(result);

    table_chart.onCSVDownload(function() {
      on_csv_download.forEach(function(callback_fn) {
        callback_fn(result);
      });
    });
  }

};


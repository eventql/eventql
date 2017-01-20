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

EventQL.SQLEditor.Table = function(elem) {
  'use strict';

  this.render = function(result, idx) {
    if (result.rows.length == 0) {
      renderEmptyResult(elem);
      return;
    }

    var tpl = TemplateUtil.getTemplate("evql-sql-editor-table-tpl");
    elem.appendChild(tpl);

    //renderTable(result);
    renderLineChart(result);
  };

  function renderTable(result) {
    var thead_elem = elem.querySelector("thead tr");
    result.columns.forEach(function(col) {
      var th_elem = document.createElement("th");
      th_elem.innerHTML = DOMUtil.escapeHTML(col);
      thead_elem.appendChild(th_elem);
    });

    var tbody_elem = elem.querySelector("tbody");
    result.rows.forEach(function(row) {
      var tr_elem = document.createElement("tr");

      row.forEach(function(cell) {
        var td_elem = document.createElement("td");
        td_elem.innerHTML = DOMUtil.escapeHTML(cell);
        tr_elem.appendChild(td_elem);
      });

      tbody_elem.appendChild(tr_elem);
    });
  }

  function renderLineChart(result) {
    var chart_elem = elem.querySelector(".chart_container");
    var chart = new EventQL.ChartPlotter(elem, {
      height: 300,
      type: "line",
      points: true,
      timeseries: true //FIXME make this configurable
    });

    chart.render(result);
  }

  function renderBarChart(result) {

  }

};


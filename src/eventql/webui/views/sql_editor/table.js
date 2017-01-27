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

EventQL.SQLEditor.ResultList.TableChartBuilder = function(elem, params) {
  'use strict';

  var data_visualizations = [
    {
      key: "table",
      title: "Table"
    },
    {
      key: "line",
      title: "Linechart"
    },
    {
      key: "bar",
      title: "Barchart"
    }
  ];

  var on_csv_download = [];
  var on_param_change = [];

  this.onCSVDownload = function(fn) {
    on_csv_download.push(fn);
  };

  this.onParamChange = function(fn) {
    on_param_change.push(fn);
  };

  this.render = function(result) {
    var tpl = TemplateUtil.getTemplate(
        "evql-sql-editor-result-table-chart-builder-tpl");
    elem.appendChild(tpl);

    initToolbar();

    var data_visualization = getDataVisualizationValue();

    var dropdown_elem = elem.querySelector("[data-control='visualization']");
    var dropdown = new EventQL.Dropdown(dropdown_elem);
    dropdown.setMenuItems(data_visualizations);
    dropdown.setValue(data_visualization);

    dropdown.onChange(function(value) {
      updateDataVisualization(value, result);
      on_param_change.forEach(function(callback_fn) {
        callback_fn({visualization: value});
      });
    });

    updateDataVisualization(data_visualization, result);
  };

  function initToolbar() {
    elem.querySelector("[data-content='result_idx']").innerHTML = params.idx + 1;

    if (params.hidable) {
      var visibility_ctrl = elem.querySelector("[data-control='visibility']");
      visibility_ctrl.addEventListener("click", function(e) {
        elem.classList.toggle("hidden");
      }, false);
    }

    var download_csv_elem = elem.querySelector("[data-control='download_csv']");
    download_csv_elem.addEventListener("click", function(e) {
      on_csv_download.forEach(function(callback_fn) {
        callback_fn();
      });
    }, false);
  }

  function updateDataVisualization(visualization, result) {
    switch (visualization) {
      case "table":
        renderTable(result);
        break;

      case "bar":
      case "line":
        buildChart(result, visualization);
        break;
    }
  }

  function renderTable(result) {
    var thead_elem = document.createElement("tr");
    result.columns.forEach(function(col) {
      var th_elem = document.createElement("th");
      th_elem.innerHTML = DOMUtil.escapeHTML(col);
      thead_elem.appendChild(th_elem);
    });

    var tbody_elem = document.createElement("tbody");
    result.rows.forEach(function(row) {
      var tr_elem = document.createElement("tr");

      row.forEach(function(cell) {
        var td_elem = document.createElement("td");
        td_elem.innerHTML = DOMUtil.escapeHTML(cell);
        tr_elem.appendChild(td_elem);
      });

      tbody_elem.appendChild(tr_elem);
    });

    var table = document.createElement("table");
    table.classList.add("evql_table");
    var thead = document.createElement("thead");
    thead.appendChild(thead_elem);
    table.appendChild(thead);
    table.appendChild(tbody_elem);

    DOMUtil.replaceContent(elem.querySelector(".result_view"), table);
  }

  function buildChart(result, chart_type) {

    var chart_cfg = {
      height: 300
    };

    switch (chart_type) {
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

    chart_cfg.type = chart_type;

    var chart_elem = document.createElement("div")
    chart_elem.classList.add("chart_container");
    DOMUtil.replaceContent(elem.querySelector(".result_view"), chart_elem);

    var chart = new EventQL.ChartPlotter(chart_elem, chart_cfg);

    try {
      chart.render(result);

    /* render chart builder errror */
    } catch (e) {
      var error_elem = document.createElement("div");
      error_elem.classList.add("message", "error");
      error_elem.innerHTML = "The chosen chart can't be drawn. Please try another chart type or select table to display the result.";
      chart_elem.appendChild(error_elem);
    }
  }

  function getDataVisualizationValue() {
    var default_value = data_visualizations[0].key;

    if (params.data_visualization) {
      for (var i = 0; i < data_visualizations.length; i++) {
        if (data_visualizations[i].key == params.data_visualization) {
          return params.data_visualization;
        }
      }
    }

    return default_value;
  }

};


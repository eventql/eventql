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

EventQL.SQLEditor.ResultToolbar = function(elem, opts) {
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

  var csv_download_callback;
  var data_visualization_callback;

  this.onCSVDownload = function(callback_fn) {
    csv_download_callback = callback_fn;
  };

  this.onDataVisualizationChange = function(callback_fn) {
    data_visualization_callback = callback_fn;
  };

  this.getDataVisualization = getDataVisualizationValue;

  this.render = function(idx) {
    var tpl = TemplateUtil.getTemplate("evql-sql-editor-result-toolbar-tpl");
    elem.appendChild(tpl);

    elem.querySelector("[data-content='result_idx']").innerHTML = idx + 1;

    if (opts.enable_hide) {
      var visibility_ctrl = elem.querySelector("[data-control='visibility']");
      visibility_ctrl.addEventListener("click", function(e) {
        elem.classList.toggle("hidden");
      }, false);
    }

    if (opts.has_controls) {
      elem.querySelector(".table_controls").classList.remove("hidden");

      initDataVisualizationControl();
      initCSVDownload();
    }
  }

  function initDataVisualizationControl() {
    var dropdown_elem = elem.querySelector("[data-control='visualization']");
    var dropdown = new EventQL.Dropdown(dropdown_elem);
    dropdown.setMenuItems(data_visualizations);
    dropdown.setValue(getDataVisualizationValue());
    dropdown.onChange(function(value) {
      if (data_visualization_callback) {
        data_visualization_callback(value);
      }
    });
  }

  function getDataVisualizationValue() {
    var default_value = data_visualizations[0].key;

    if (opts.data_visualization) {
      for (var i = 0; i < data_visualizations.length; i++) {
        if (data_visualizations[i].key == opts.data_visualization) {
          return opts.data_visualization;
        }
      }
    }

    return default_value;
  }

  function initCSVDownload() {
    var download_csv_elem = elem.querySelector("[data-control='download_csv']");
    download_csv_elem.addEventListener("click", function(e) {
      if (csv_download_callback) {
        csv_download_callback();
      }
    }, false);
  }

};

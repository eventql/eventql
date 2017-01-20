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

EventQL.SQLEditor.ResultList = function(elem) {
  'use strict';

  const CHART_COLUMN_NAME = "__chart";

  var settings = {};

  var on_csv_download = [];
  var on_settings_update = [];

  this.render = function(results) {
    DOMUtil.clearChildren(elem);
    elem.setAttribute("data-result-count", results.length);

    for (var i = 0; i < results.length; i++) {
      if (results[i].columns.length == 0) {
        continue;
      }

      var result_elem = document.createElement("div");
      elem.appendChild(result_elem);

      //render result chart
      if (results[i].columns[0] == CHART_COLUMN_NAME) {
        if (results[i].rows.length == 0) { //error: no svg returned
          continue;
        }

        var chart_result = new EventQL.SQLEditor.Chart(result_elem);
        chart_result.render(results[i].rows[0][0], i);

      //render result table
      } else {
        var table_result = new EventQL.SQLEditor.Table(
              result_elem,
              settings[i]);
          //table_result.onSettingsChange(updateResultSettingsByIndex);
          table_result.render(results[i], i);
      }
    }
  };

  this.onCSVDownload = function(fn) {
    on_csv_download.push(fn);
  };

  this.onSettingsUpdate = function(fn) {
    on_settings_update.push(fn);
  };

  this.getSettings = function() {
    return settings;
  };

  this.setSettings = function(new_settings) {
    if (new_settings) {
      settings = new_settings;
    }
  };

  function updateResultSettingsByIndex(index, result_settings) {
    settings[index] = result_settings;

    on_settings_update.forEach(function(fn) {
      fn();
    });
  }
};


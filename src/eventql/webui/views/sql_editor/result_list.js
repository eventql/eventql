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

  var settings = {};

  var on_csv_download = [];
  var on_settings_update = [];

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

  this.render = function(results) {
    DOMUtil.clearChildren(elem);
    elem.setAttribute("data-result-count", results.length);

    for (var i = 0; i < results.length; i++) {
      var result_elem = document.createElement("div");
      elem.appendChild(result_elem);

      switch (results[i].type) {
        case "chart":
          var chart_result = new EventQl.SQLEditor.Chart(results[i], i);
          chart_result.render(result_elem);
          break;

        case "table":
          var table_result = new EventQL.SQLEditor.Table(
              result_elem,
              settings[i]);
          //table_result.onSettingsChange(updateResultSettingsByIndex);
          table_result.render(results[i], i);
          break;

      }
    }
  };

  function updateResultSettingsByIndex(index, result_settings) {
    settings[index] = result_settings;

    on_settings_update.forEach(function(fn) {
      fn();
    });
  }
};


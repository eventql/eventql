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

EventQL.SQLEditor = function(elem, params) {
  'use strict';

  var query_mgr = EventSourceHandler();
  var path = params.path;

  this.initialize = function() {
    var tpl = TemplateUtil.getTemplate("evql-sql-editor-tpl");
    elem.appendChild(tpl);

    initExecuteTooltip();
    initCodeEditor();
  };

  this.changePath = function(new_path, new_route) {
    var allowed = ["query"];
    var diff = URLUtil.comparePaths(path, new_path);
    if (diff.path || ArrayUtil.setDifference(diff.params, allowed).length) {
      return false;
    }

    path = new_path;

    var query_param = URLUtil.getParamValue(params.path, "query");
    executeQuery(query_param);
    return true;
  }

  this.destroy = function() {
    query_mgr.closeAll();
  };

  function initExecuteTooltip() {
    var tooltip = new EventQL.Tooltip(
        elem.querySelector("[data-control='execute_tooltip']"),
        elem.querySelector("button[data-action='execute-query']"),
        {});
  }

  function initCodeEditor() {
    var editor_elem = elem.querySelector("[data-content='codeeditor']");
    var editor = new EventQL.CodeEditor(editor_elem, {
      language: "text/x-chartsql",
      resizable: "vertical"
    });
    editor.render();

    editor.setExecuteCallback(function(query_str) {
      EventQL.navigateTo(URLUtil.addOrModifyParam(path, "query", query_str));
    });

    var execute_btn = elem.querySelector("button[data-action='execute-query']");
    DOMUtil.onClick(execute_btn, function() {
      EventQL.navigateTo(URLUtil.addOrModifyParam(path, "query", editor.getValue()));
    });

    var query_str_param = URLUtil.getParamValue(params.path, "query");
    if (query_str_param) {
      editor.setValue(query_str_param);
      executeQuery(query_str_param);
    }
  }

  var executeQuery = function(query_str) {
    //displayQueryProgress(); FIXME

    query_mgr.closeAll();
    var query = query_mgr.add(
      "sql_query",
      params.app.api_stubs.evql.executeSQLSSE(query_str));

    query.addEventListener('result', function(e) {
      query_mgr.close("sql_query");
      var data = JSON.parse(e.data);
      displayQueryResult(data.results);
    });

    query.addEventListener('query_error', function(e) {
      query_mgr.close("sql_query");
      displayQueryError(JSON.parse(e.data).error);
    });

    query.addEventListener('error', function(e) {
      query_mgr.close("sql_query");
      displayQueryError("Server Error");
    });

    query.addEventListener('status', function(e) {
      //displayQueryProgress(JSON.parse(e.data)); FIXME
    });
  };

  var displayQueryResult = function(result) {
    var result_list_elem = elem.querySelector(".result");
    var result_list = new EventQL.SQLEditor.ResultList(result_list_elem);
    result_list.render(result);
    result_list.onCSVDownload(function(result) {
      downloadCSV(result.columns, result.rows);
    });
  };

  var displayQueryError = function(error) {
    var error_msg = zTemplateUtil.getTemplate("zbase_sql_editor_error_msg_tpl");
    error_msg.querySelector(".info").innerHTML = error;
    zDomUtil.replaceContent(
        elem.querySelector(".zbase_sql_editor_result_pane"),
        error_msg);
  };

  var displayQueryProgress = function(progress) {
    QueryProgressWidget.render(
        elem.querySelector(".zbase_sql_editor_result_pane"),
        progress);
  };

  var renderError = function(header) {
    var error_msg = new ZFE.ErrorMessage(elem, {
      header: header,
      path: params.path
    });
    error_msg.render();
  };

  var downloadCSV = function(columns, rows) {
    var csv = zCSVUtil.toCSV({}, columns, rows);
    var name = doc.file_name ? doc.file_name + ".csv" : "sql_query.csv";
    zDownload(csv, name, "text/csv");
  };
};

EventQL.views["eventql.sql_editor"] = EventQL.SQLEditor;


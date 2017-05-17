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

EventQL.DatabaseList = function(elem, params) {
  'use strict';

  var query_mgr = EventSourceHandler();

  this.initialize = function() {
    var tpl = TemplateUtil.getTemplate("evql-database-list-tpl");
    elem.appendChild(tpl);

    initCreateDatabaseControl();
    fetchData();
  };

  function initCreateDatabaseControl() {
    var modal_elem = elem.querySelector(".evql_modal");
    var modal = new EventQL.Modal(modal_elem);
    modal.onSubmit(function() {
      var db_name = modal_elem.querySelector("input[name='database']").value;
      createDatabase(db_name);
    });

    var control = elem.querySelector("button[data-control='create_database']");
    control.addEventListener("click", function(e) {
      modal.show();
    }, false);
  }

  function fetchData() {
    var query_str = "SHOW TABLES;";
    var query_id = "show_databases";

    var query = query_mgr.add(
        query_id,
        params.app.api_stubs.evql.executeSQLSSE(query_str));

    query.addEventListener('result', function(e) {
      query_mgr.close(query_id);
      var result = JSON.parse(e.data).results;

      if (result[0].rows.length > 0) {
        renderDatabaseList(result);
      } else {
        renderEmptyResult();
      }
    });

    query.addEventListener('query_error', function(e) {
      query_mgr.close(query_id);
      renderError("Query Error: " + JSON.parse(e.data).error);
    });

    query.addEventListener('error', function(e) {
      query_mgr.close(query_id);
      renderError("Server Error");
    });

    query.addEventListener('status', function(e) {
      ///renderQueryProgress(JSON.parse(e.data));
    });
  }

  function renderDatabaseList(result) {
    var tbody_elem = elem.querySelector("table.database_list tbody");

    result[0].rows.forEach(function(row) {
      var database_name = row[0];
      var tr_elem = document.createElement("tr");

      var td_elem = document.createElement("td");
      td_elem.innerHTML = DOMUtil.escapeHTML(database_name);
      tr_elem.appendChild(td_elem);

      tr_elem.addEventListener("click", function(e) {
        EventQL.navigateTo("/ui/" + encodeURIComponent(database_name));
      }, false);

      tbody_elem.appendChild(tr_elem);
    });
  }

  function createDatabase(db_name) {
    var query_str = "CREATE DATABASE " + db_name + ";";
    var query_id = "create_database";

    var query = query_mgr.add(
        query_id,
         params.app.api_stubs.evql.executeSQLSSE(query_str));

    query.addEventListener('result', function(e) {
      query_mgr.close(query_id);
      var data = JSON.parse(e.data);
      fetchData();
    });

    query.addEventListener('query_error', function(e) {
      query_mgr.close(query_id);
      console.log("error", JSON.parse(e.data).error);
      //renderQueryError(JSON.parse(e.data).error);
    });

    query.addEventListener('error', function(e) {
      query_mgr.close(query_id);
      //renderQueryError("Server Error");
    });

    query.addEventListener('status', function(e) {
      ///renderQueryProgress(JSON.parse(e.data));
    });
  }

  function renderEmptyResult() {
    var result_tpl = TemplateUtil.getTemplate(
        "evql-database-list-empty-result-tpl");

    var result_elem = elem.querySelector(".result");
    DOMUtil.replaceContent(result_elem, result_tpl);
  }

  function renderError(error_txt) {
    var error_elem = elem.querySelector(".result");
    var error_message = new EventQL.ErrorMessage(error_elem, {
      info: error_txt
    });
    error_message.render();
  }

};

EventQL.views["eventql.database_list"] = EventQL.DatabaseList;


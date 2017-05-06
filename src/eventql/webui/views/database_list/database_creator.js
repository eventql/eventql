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

EventQL.DatabaseList.DatabaseCreator = function(elem, query_mgr, params) {
  'use strict';

  var change_callback;
  var modal;

  this.setChangeCallback = function(callback_fn) {
    change_callback = callback_fn;
  };

  this.init = function(ctrl_elem) {
    var tpl = TemplateUtil.getTemplate("evql-database-list-database-creator-tpl");
    elem.appendChild(tpl);

    var modal_elem = elem.querySelector(".evql_modal");
    modal = new EventQL.Modal(modal_elem);

    var input_elem = modal_elem.querySelector("input[name='database']");
    DOMUtil.onEnter(input_elem, function() {
      var db_name = input_elem.value;
      createDatabase(db_name);
    });

    modal.onSubmit(function() {
      var db_name = input_elem.value;
      createDatabase(db_name);
    });

    ctrl_elem.addEventListener("click", function(e) {
      modal.show();
      input_elem.focus();
    }, false);
  };

  function createDatabase(db_name) {
    var query_str = "CREATE DATABASE " + db_name + ";";
    var query_id = "create_database";

    var query = query_mgr.add(
        query_id,
         params.app.api_stubs.evql.executeSQLSSE(query_str));

    query.addEventListener('result', function(e) {
      query_mgr.close(query_id);
      var data = JSON.parse(e.data);

      modal.close();
      if (change_callback) {
        change_callback(data);
      }
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
};

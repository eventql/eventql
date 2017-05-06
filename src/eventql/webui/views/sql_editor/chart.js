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

EventQL.SQLEditor.ResultList.Chart = function(elem, params) {
  'use strict';

  this.render = function(svg) {
    var tpl = TemplateUtil.getTemplate("evql-sql-editor-result-chart-tpl");
    elem.appendChild(tpl);

    elem.querySelector("[data-content='result_idx']").innerHTML = params.idx + 1;
    if (params.hidable) {
      var visibility_ctrl = elem.querySelector("[data-control='visibility']");
      visibility_ctrl.addEventListener("click", function(e) {
        elem.classList.toggle("hidden");
      }, false);
    }

    var chart_elem = elem.querySelector(".chart_container");
    chart_elem.innerHTML = svg;
  }

};

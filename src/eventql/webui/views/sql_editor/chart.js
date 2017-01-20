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

EventQL.SQLEditor.Chart = function(elem) {
  'use strict';

  this.render = function(svg, idx) {
    var tpl = TemplateUtil.getTemplate("evql-sql-editor-chart-tpl");
    elem.appendChild(tpl);

    renderToolbar(idx);

    elem.querySelector(".result_view").innerHTML = svg;
  };

  var renderToolbar = function(idx) {
    elem.querySelector(".result_index").innerHTML = idx + 1;

   // DOMUtil.onClick(bar.querySelector(".display_icon"), function() {
   //   //if (bar.hasAttribute('data-active')) {
   //   //  bar.removeAttribute('data-active');
   //   //} else {
   //   //  bar.setAttribute('data-active', 'active');
   //   //}
   // });

  }

}

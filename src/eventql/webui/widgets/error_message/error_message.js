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

EventQL.ErrorMessage = function(elem, params) {
  const DEFAULT_HEADER = "An error occured";

  this.render = function() {
    var tpl = TemplateUtil.getTemplate("evql-error-message-tpl");

    tpl.querySelector("h1").innerHTML =
      DOMUtil.escapeHTML(params.header) || DEFAULT_HEADER;
    tpl.querySelector(".info").innerHTML =
      DOMUtil.escapeHTML(params.info) || "";

    if (params.path) {
      var reload_btn = tpl.querySelector("[data-control='reload']");
      reload_btn.href = params.path;
      reload_btn.classList.remove("hidden");
    }

    DOMUtil.replaceContent(elem, tpl);
  }
};

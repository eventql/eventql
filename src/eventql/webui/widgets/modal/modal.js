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

EventQL.Modal = function(elem) {
  'use strict';

  var on_submit = [];

  var hide_ctrl = elem.querySelector("[data-control='hide']");
  if (hide_ctrl) {
    hide_ctrl.addEventListener("click", close, false);
  }

  var submit_ctrl = elem.querySelector("[data-control='submit']");
  if (submit_ctrl) {
    submit_ctrl.addEventListener("click", function(e) {
      on_submit.forEach(function(fn) {
        fn();
      });
    }, false);
  }

  var on_escape = function(e) {
    if (e.keyCode == 27) {
      close();
    }
  };

  var on_click = function(e) {
    e.stopPropagation();
  };

  this.onSubmit = function(callback_fn) {
    on_submit.push(callback_fn);
  };

  this.show = function() {
    elem.classList.add("active");

    document.addEventListener('keyup', on_escape, false);
    elem.addEventListener("click", close, false);

    var modal_window = elem.querySelector(".modal_window");
    modal_window.addEventListener("click", on_click, false);
  };

  this.close = function() {
    close();
  };

  function close() {
    elem.classList.remove("active");

    document.removeEventListener('keyup', on_escape, false);
    elem.removeEventListener("click", close, false);

    var modal_window = elem.querySelector(".modal_window");
    modal_window.removeEventListener("click", on_click, false);
  }
};


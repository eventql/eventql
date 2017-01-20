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
EventQL.Tooltip = function(elem, target) {
  'use strict';

  elem.classList.add("evql_tooltip");

  target.addEventListener('mouseover', function() {
    show();
  }, false);

  target.addEventListener('mouseout', function() {
    hide();
  }, false);

  function show() {
    elem.classList.add("active");

    elem.style.left = getLeftValue() + "px";
    elem.style.top = getTopValue() + "px";
  }

  function hide() {
    elem.classList.remove("active");
  }

  function getLeftValue () {
    var width = elem.offsetWidth;
    var target_width = target.offsetWidth;
    var target_middle = Math.round(target_width / 2);
    var pos = target.getBoundingClientRect();
    var scroll_x = window.scrollX;

    var elem_position = elem.getAttribute("data-position");
    var pointer_position = elem.getAttribute("data-pointer");

    switch (elem_position) {
      case "right":
        return pos.left + target_width + 7;

      case "left":
        return pos.left - width - 7
    }

    switch (pointer_position) {
      case "right":
        return (pos.left - width + target_middle + 15);

      case "left":
        return pos.left - ((width - target_width) / 2);
    }

    /* the tooltip is bigger than remaining window space */
    if (window.innerWidth - (pos.left + target_middle - 15) < width) {
      elem.setAttribute("data-pointer", "right");
      return (pos.left - width + target_middle + 16);
    }

    return (pos.left + target_middle - 15);
  }

  function getTopValue() {
    var pos = target.getBoundingClientRect();
    var scroll_x = window.scrollX;

    var elem_position = elem.getAttribute("data-position");
    switch (elem_position) {
      case "bottom":
        return pos.top + pos.height + scroll_x + 8;

      case "left":
      case "right":
        return pos.top + scroll_x;
    }

    return (pos.top - elem.offsetHeight + scroll_x - 8);
  }
};

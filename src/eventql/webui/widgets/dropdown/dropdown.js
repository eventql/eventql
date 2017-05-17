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

EventQL.Dropdown = function(elem) {
  'use strict';

  var change_callbacks = [];
  var menu_items = [];
  var selected_item_key;

  var tpl = TemplateUtil.getTemplate("evql-dropdown-tpl");
  elem.appendChild(tpl);

  var toggle_btn = elem.querySelector("button");
  toggle_btn.addEventListener("click", function(e) {
    e.stopPropagation();
    toggleMenu();
  }, false);

  var onClose = function(e) {
    e.stopPropagation();
    hideMenu();
  };

  var onUnload = function() {
    document.removeEventListener("click", onClose, false);
    document.removeEventListener("evql-unload", onUnload, false);
  };

  document.addEventListener("click", onClose, false);
  document.addEventListener("evql-unload", onUnload, false);

  this.onChange = function(callback_fn) {
    change_callbacks.push(callback_fn);
  }

  this.setValue = function(key) {
    selected_item_key = key;

    menu_items.forEach(function(item) {
      if (selected_item_key == item.key) {
        elem.querySelector("button span").innerHTML =
            DOMUtil.escapeHTML(item.title);

        return false;
      }
    });

    var selected_elem = elem.querySelector("ul li.active");
    if (selected_elem) {
      selected_elem.classList.remove("active");
    }

    elem
      .querySelector("ul li[data-key='" + selected_item_key  +"']")
      .classList.add("active");

    change_callbacks.forEach(function(callback_fn) {
      callback_fn(selected_item_key);
    });
  }

  this.setMenuItems = function(dropdown_menu_items) {
    menu_items = dropdown_menu_items;

    var menu = elem.querySelector("ul");;
    var this_ = this;
    menu_items.forEach(function(item) {
      var li = document.createElement("li");
      li.setAttribute("data-key", DOMUtil.escapeXML(item.key.toString()));
      li.innerHTML = DOMUtil.escapeHTML(item.title);

      li.addEventListener("click", function(e) {
        this_.setValue(item.key);
        hideMenu();
      }, false);

      menu.appendChild(li);
    });
  }

  function toggleMenu() {
    if (elem.querySelector("ul").classList.contains("active")) {
      hideMenu();
    } else {
      showMenu();
    }
  }

  function showMenu() {
    elem.querySelector("ul").classList.add("active");
  }

  function hideMenu() {
    elem.querySelector("ul").classList.remove("active");
  }

};


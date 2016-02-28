/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Laura Schlimmer
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
var MenuComponent = function() {
  this.createdCallback = function() {
    var option = this.getAttribute('data-option');

    if (option == 'accordion') {
      this.handleAccordionMenu();
    }

    this.observeItems();
  };

  this.attributeChangedCallback = function(attr, old_val, new_val) {
    if (attr == 'data-option' && new_val == 'accordion') {
      this.handleAccordionMenu();
      return;
    }

    if (attr == 'data-resolved') {
      this.observeItems();
    }
  };

  this.handleAccordionMenu = function() {
    var base = this;
    var title_elems = this.querySelectorAll("z-menu-title");

    for (var i = 0; i < title_elems.length; i++) {
      title_elems[i].addEventListener('click', function() {
        base.toggleAccordionSection(this);
      }, false);
    }
  };

  this.observeItems = function() {
    var base = this;
    var items = this.querySelectorAll("z-menu-item");

    for (var i = 0; i < items.length; i++) {
      items[i].addEventListener('click', function() {
        base.onItemClick(this);
      }, false);
    }
  }

  this.onItemClick = function(item) {
    var active_items = this.querySelectorAll("z-menu-item[data-active]");

    for (var i = 0; i < active_items.length; ++i) {
      active_items[i].removeAttribute("data-active");
    }

    item.setAttribute("data-active", "active");

    var ev = new CustomEvent("z-menu-item-click", {
      bubbles: true,
      cancelable: true
    });
    item.dispatchEvent(ev);
  }

  this.toggleAccordionSection = function(elem) {
    var active_section = this.querySelector("z-menu-section[data-active]");

    if (!(elem.parentNode.hasAttribute('data-active'))) {
      elem.parentNode.setAttribute('data-active', 'active');
    }

    if (active_section) {
      active_section.removeAttribute('data-active');
    }
  }
};

var proto = Object.create(HTMLElement.prototype);
MenuComponent.apply(proto);
document.registerElement("z-menu", { prototype: proto });

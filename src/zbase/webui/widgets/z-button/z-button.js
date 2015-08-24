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
var ButtonComponent = function() {
  this.createdCallback = function() {
    this.addEventListener("click", function() {
      var ev = new CustomEvent("fn-button-click", {
        bubbles: true,
        cancelable: true
      });

      this.dispatchEvent(ev);
    }, false);
  };
};

var proto = Object.create(HTMLElement.prototype);
ButtonComponent.apply(proto);
document.registerElement("z-button", { prototype: proto });

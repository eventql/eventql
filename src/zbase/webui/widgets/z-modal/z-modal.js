/**
  This file is part of the "FnordMetric" project
    Copyright (c) 2014 Laura Schlimmer
    Copyright (c) 2014 Paul Asmuth, Google Inc.

  FnordMetric is free software: you can redistribute it and/or modify it under
  the terms of the GNU General Public License v3.0. You should have received a
  copy of the GNU General Public License along with this program. If not, see
  <http://www.gnu.org/licenses/>.
**/
var ModalComponent = function() {
  this.createdCallback = function() {
    var _this = this;
    this.onclick = function(e) {
      _this.close();
    };

    this.querySelector("z-modal-close-icon").onclick = function() {
      _this.close();
    };

    this.querySelector("z-modal-box").onclick = function(e) {
      e.stopPropagation();
    };

  };

  this.show = function() {
    this.setAttribute('data-active', 'active');

    var on_escape = function(e) {
      if (e.keyCode == 27) {
        this.close();
      }
    };

    //place modal box
    var modal_box = this.querySelector("z-modal-box");
    var height = modal_box.getBoundingClientRect().height;
    modal_box.style.top = ((window.innerHeight - height) / 4) + "px";

    this.escape_listener = on_escape.bind(this);
    document.addEventListener('keyup', this.escape_listener, false);
  };

  this.close = function() {
    this.setAttribute('data-active', "");
    document.removeEventListener('keyup', this.escape_listener, false);
    var ev = new CustomEvent("z-modal-close", {
      bubbles: true,
      cancelable: true
    });

    this.dispatchEvent(ev);
  };
};

var proto = Object.create(HTMLElement.prototype);
ModalComponent.apply(proto);
document.registerElement("z-modal", { prototype: proto });

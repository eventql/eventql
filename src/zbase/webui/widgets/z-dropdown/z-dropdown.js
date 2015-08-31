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
var DropDownComponent = function() {

  this.createdCallback = function() {
    var base = this;

    // dropdown items
    var items = this.querySelector("z-dropdown-items");
    if (items) {
      var cloned = items.cloneNode(true);
      this.setDropdownItems(cloned.children);
    }

    // header
    var header_elem = this.querySelector("z-dropdown-header");
    if (!header_elem) {
      header_elem = document.createElement("z-dropdown-header");
      this.insertBefore(header_elem, this.firstChild);
    }

    //render header value
    if (!header_elem.querySelector("z-drodpown-header-value")) {
      header_elem.appendChild(document.createElement(
        "z-dropdown-header-value"));
    }

    //render header icon
    if (!header_elem.querySelector("z-dropdown-header-icon")) {
      var icon = document.createElement("z-dropdown-header-icon");
      icon.innerHTML = "<i class='fa fa-caret-down'></i>";
      header_elem.appendChild(icon);
    }

    //render apply button
    if (this.hasAttribute("data-multiselect")) {
      var button_container = document.createElement("z-dropdown-button");
      var button = document.createElement("button");
      button.className = "z-button primary"
      button.innerHTML = "Apply";
      button_container.appendChild(button);
      this.querySelector("z-dropdown-flyout").insertBefore(
        button_container,
        items);

      button.addEventListener("click", base.hideDropdown.bind(base), false);
    }

    header_elem.addEventListener('click', function(e) {
      e.stopPropagation();
      base.toggleDropdown();
    }, false);

    if (this.hasAttribute('data-hover')) {
      this.addEventListener(
        'mouseover',
        base.showDropdown.bind(base),
        false);

      this.addEventListener(
        'mouseout',
        base.hideDropdown.bind(base),
        false);
    }

    //close dropdown only if click event is outside
    this.addEventListener('click', function(e) {
      e.stopPropagation();
    }, false);

    this.__applyAttributes();
  };

  this.detachedCallback = function() {
    this.hideDropdown();
  }

  this.attributeChangedCallback = function(attr, old_val, new_val) {
    this.__applyAttributes();
  };

  /**
    * Returns either the selected items data-value attr or text content
    *
    * @return {String} csv
    */
  this.getValue = function() {
    var items = this.querySelectorAll("z-dropdown-item[data-selected]");
    var value = [];

    for (var i = 0; i < items.length; i++) {
      if (items[i].hasAttribute('data-value')) {
        value.push(items[i].getAttribute('data-value'));
      } else {
        value.push(items[i].textContent);
      }
    }

    return value.join(",");
  };

  /**
    * Marks for each value the matching dropdown-item as selected
    *
    * @param {Array} values
    */
  this.setValue = function(values) {
    var _this = this;

    values.forEach(function(v) {
      var item = _this.querySelector("z-dropdown-item[data-value='" + v + "']");

      if (item) {
        item.setAttribute('data-selected', 'selected');

        var checkbox = item.querySelector("z-checkbox");

        if (checkbox) {
          checkbox.setAttribute('data-active', 'active');
        }
      }
    });

    this.__setHeaderValue();
  }

  this.setDropdownItems = function(items) {
    var base = this;

    var elem = this.querySelector("z-dropdown-items");
    elem.innerHTML = "";

    for (var i = 0; i < items.length; i++) {
      items[i].addEventListener('click', function(e) {
        e.stopPropagation;
        base.__onItemClick(this);
      }, false);

      elem.appendChild(items[i]);
    }
  };

  this.toggleDropdown = function() {
    if (this.hasAttribute('data-active')) {
     this.hideDropdown();
    } else {
      this.showDropdown();
    }
  };

  this.showDropdown = function() {
    if (this.classList.contains("disabled")) {
      return;
    }

    if (this.getAttribute('data-active') == 'active') {
      return;
    }

    this.closeAllDropdowns();
    this.setAttribute('data-active', 'active');

    //this.setXValue();

    this.__setKeyNavigation();
  };

  this.hideDropdown = function() {
    if (this.getAttribute('data-active') != 'active') {
      return;
    }

    this.__unsetKeyNavigation();
    this.removeAttribute('data-active');

    var ev = new CustomEvent('z-dropdown-close', {
      bubbles: true,
      cancelable: true
    });

    this.dispatchEvent(ev);
    if (this.hasAttribute("data-multiselect")) {
      this.__fireChangeEvent();
    }
  };

  /**
    * Close all open dropdowns
    */
  this.closeAllDropdowns = function(callback) {
    var active_items = document.querySelectorAll("z-dropdown[data-active]");

    for (var i = 0; i < active_items.length; i++) {
      if (typeof active_items[i].hideDropdown == 'function') {
        active_items[i].hideDropdown();
      }
    }

    if (callback) callback();
  };

  this.findItemByValue = function(value) {
    return (this.querySelector("z-dropdown-item[data-value='" + value + "']"));
  };

  /*************************** PRIVATE **********************************/

  this.__applyAttributes = function() {
    this.__setHeaderValue();
  }

  // Copies the selected items html to the header_elem
  this.__setHeaderValue = function() {
    var header = this.querySelector("z-dropdown-header-value");

    if (header.hasAttribute('data-static')) {
      return;
    }

    var innerHTML = "";

    var selected_items = this.querySelectorAll("z-dropdown-item[data-selected]");
    if (selected_items.length == 0) {
      var preselection = this.getAttribute('data-preselected');
      //set preselected item
      if (preselection) {
        var preselected_item = this.findItemByValue(preselection);

        preselected_item.setAttribute('data-selected', 'selected');
        innerHTML = preselected_item.innerHTML;

      } else {
        //z-dropdown has initial not selectable header
        if (header.innerHTML.length > 0) {
          return;
        }

        //set first z-dropdown-item as header value
        var first_item = this.querySelector("z-dropdown-item");
        if (first_item) {
          innerHTML = first_item.innerHTML;
        }
      }
    }

    if (selected_items.length == 1) {
      var selected_item = selected_items[0];
      if (!selected_item) {
      } else {
        //set items input value as html header
        if (selected_item.hasAttribute('data-input-select')) {
          var input = selected_item.querySelector("input");
          innerHTML = input.value;

        } else {
          //set selected_item's html as header (standard case)
          innerHTML = selected_item.innerHTML;
        }
      }
    }

    if (selected_items.length > 1) {
      for (var i = 0; i < selected_items.length; i++) {
        if (i > 0) {
          innerHTML += ", ";
        }

        innerHTML += selected_items[i].innerText || selected_items[i].textContent;
      }
    }

    header.innerHTML = innerHTML;
    var checkbox = header.querySelector("z-checkbox");
    if (checkbox) checkbox.remove();
  };

  this.unselectItem = function(item) {
    if (!item) {return;}
    item.removeAttribute('data-selected');
    var checkbox = item.querySelector("z-checkbox");
    if (checkbox) {
      checkbox.removeAttribute('data-active');
    }
  };

  this.selectItem = function(item) {
    if (!item) {return;}
    item.setAttribute('data-selected', 'selected');
    var checkbox = item.querySelector("z-checkbox");
    if (checkbox) {
      checkbox.setAttribute('data-active', 'active');
    }
  };


  this.__onItemClick = function(item) {
    if (!item) {
      return;
    }

    var selected = true;
    var checkbox =
      item.querySelector("z-checkbox") || item.querySelector("[data-check]");

    //multi-selectable dropdown with checkboxes
    if (checkbox != null) {
      //unselect item
      if (item.hasAttribute('data-selected')) {
        selected = false;
        this.unselectItem(item);

        //not completely unselectable dropdown
        if (this.hasAttribute('data-force-select') &&
            !this.querySelector('z-dropdown-item[data-selected]')) {

          var default_select = this.querySelector('z-dropdown-item[data-default]');
          if (default_select) {
            default_select.setAttribute('data-selected', 'selected');
            checkbox = default_select.querySelector('z-checkbox');

            if (checkbox) {
              checkbox.setAttribute('data-active', 'active');
            }
          }
        }
      }
    } else {
      //unselect previously selected item
      this.unselectItem(this.querySelector("z-dropdown-item[data-selected]"));
    }

    if (selected) {
      this.selectItem(item);
    }

    this.__fireItemChangedEvent({selected: selected}, item);
    if (this.hasAttribute("data-multiselect")) {
      return;
    }

    this.hideDropdown();
    this.__fireChangeEvent();
  };

  this.__fireItemChangedEvent = function(detail, item) {
    var click_ev = new CustomEvent("item-change", {
      detail: detail,
      bubbles: true,
      cancelable: true
    });

    item.dispatchEvent(click_ev);
  };

  this.__fireChangeEvent = function() {
    var change_ev = new CustomEvent("change", {
      detail : {value: this.getValue()},
      bubbles: true,
      cancelable: true
    });

    console.log(change_ev);
    this.dispatchEvent(change_ev);
  };


  this.__setKeyNavigation = function() {
    var base = this;
    this.__unsetKeyNavigation();

    this.keydown_handler = function(e) {
      switch (e.keyCode) {
        case 38:
          base.__keyNavigation('up');
          e.preventDefault();
          return false;

        case 40:
          base.__keyNavigation('down');
          e.preventDefault();
          return false;

        case 13:
          base.__onItemClick(base.querySelector("z-dropdown-item.hover"), true);
          e.preventDefault();
          return false;

        case 27:
          base.hideDropdown();
          e.preventDefault();
          return false;

        default:
          return true;
      }
    };

    window.addEventListener('keydown', this.keydown_handler, false);

    this.click_handler = function(e) {
      base.hideDropdown();
    };

    window.addEventListener('click', this.click_handler, false);
  };

  this.__unsetKeyNavigation = function() {
    window.removeEventListener('keydown', this.keydown_handler, false);
    window.removeEventListener('click', this.click_handler, false);
  };

  this.__keyNavigation = function(direction) {
    var items = this.querySelector("z-dropdown-items");
    var visibleScrollbar = items.scrollHeight > items.offsetHeight;

    var hover_elem = this.querySelector("z-dropdown-item.hover");
    var ref_elem;
    var elem = null;
    var items = this.querySelectorAll("z-dropdown-item");

    if (!hover_elem) {
      if (direction == 'down') {
        //filtered Dropdown can contain hidden items
        if (items[0].classList.contains('hidden')) {
          hover_elem = items[0];
        } else {
          items[0].classList.add('hover');
          return;
        }
      }
    }

    ref_elem = hover_elem;
    for (var i = 0; i < items.length; i++) {

      //down
      if (direction == 'down') {
        elem = ref_elem.nextElementSibling;

        //wrap around and mark first item
        if (!elem) {
          elem = items[0];

          if (visibleScrollbar) {
            var d_items = this.querySelector("z-dropdown-items");
            d_items.scrollTop = 0;
          }

        } else {

          if (visibleScrollbar) {
            var d_items = this.querySelector("z-dropdown-items");
            d_items.scrollTop = d_items.scrollTop + (d_items.offsetHeight / items.length);
          }
        }

      //up
      } else {
        elem = ref_elem.previousElementSibling;

        //wrap around and mark last item
        if (!elem) {
          elem = items[items.length - 1];

          if (visibleScrollbar) {
            var d_items = this.querySelector("z-dropdown-items");
            d_items.scrollTop = d_items.scrollHeight;
          }

        } else {

          if (visibleScrollbar) {
            var d_items = this.querySelector("z-dropdown-items");
            d_items.scrollTop = d_items.scrollTop - (d_items.offsetHeight / items.length);
          }

        }
      }

      if (elem.classList.contains('hidden')) {
        ref_elem = elem;
      } else {
        elem.classList.add('hover');
        hover_elem.classList.remove('hover');
        return;
      }
    }
  };

 };

var proto = Object.create(HTMLElement.prototype);
DropDownComponent.apply(proto);
document.registerElement("z-dropdown", { prototype: proto });

/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Laura Schlimmer
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
var DateRangePickerComponent = function() {
  this.createdCallback = function() {
    var tpl = $.getTemplate(
        "widgets/z-daterangepicker",
        "z-daterangepicker-base-tpl");

    this.appendChild(tpl);

    //you may want to customize this
    this.selectableRanges = {
      'custom' : 'Custom',
      0 : 'Today',
      86400000 : 'Yesterday',
      604800000 : 'Last 7 Days',
      2592000000 : 'Last 30 Days'
    }

    var _this = this;


    this.querySelector("z-daterangepicker-field")
      .addEventListener("click", function(e) {
          e.stopPropagation();
          _this.toggleWidgetVisibility();
      }, false);

    this.querySelector("z-daterangepicker-widget")
      .addEventListener("click", function(e) {
            e.stopPropagation();
      }, false);

    //cancel
    this.querySelector("button.cancel").addEventListener("click", function() {
      _this.toggleWidgetVisibility();
      _this.setWidget();
    }, false);

    //apply
    this.querySelector("button.submit").addEventListener("click", function() {
      _this.applied.from = _this.editing.from;
      _this.applied.until = _this.editing.until;
      _this.toggleWidgetVisibility();
      _this.fireSelectEvent();
      _this.renderPreview();
    }, false);

    var dropdown = this.querySelector("z-dropdown");
    dropdown.closeActiveItems = function(callback) {
      callback();
    }

    //range select
    dropdown.addEventListener('change', function(e) {
      e.stopPropagation();
      _this.onDropdownChange(this.getValue());
    }, false);

    //set from
    this.querySelector("input.from").addEventListener("click", function() {
      _this.onFieldClick("from");
    }, false);

    //set until
    this.querySelector("input.until").addEventListener("click", function() {
      _this.onFieldClick("until");
    }, false);

    this.querySelector("z-calendar-group").addEventListener('select', function(e) {
        _this.onCalendarSelect(e.detail.timestamp);
    }, false);


    if (this.hasAttribute("data-from") && this.hasAttribute("data-until")) {
      this.setWidget();
      this.renderPreview();
    }
  };

  this.setValue = function(from, until) {
    var from = DateUtil.getStartOfDay(from);
    var until = DateUtil.getStartOfDay(until);

    this.setAttribute("data-from", from);
    this.setAttribute("data-until", until);

    this.applied = {
      'from' : from,
      'until' : until
    };

    this.setWidget();
    this.renderPreview();
  };

  this.getValue = function() {
    return this.applied;
  };


  this.setWidget = function() {
    var from = this.applied.from;
    var until = this.applied.until;

    this.editing = {
      'from' : from,
      'until' : until,
      'select' : 'from'
    };

    this.renderCalendar(from, until);
    this.renderDropdown();
    this.setFields(from, until);
    this.setDateRange(from, until);
  };

  this.renderPreview = function() {
    this.querySelector(".field_values").innerHTML =
        this.getDateDescription(this.applied.from) + " - " +
        this.getDateDescription(this.applied.until);
  };

  this.renderCalendar = function(from, until, calendar_start) {
    var calendar = this.querySelector("z-calendar-group");
    var start = (typeof calendar_start !== 'undefined') ?
        calendar_start : this.getCalendarStart(from, until);

    calendar.setAttribute('data-from', from);
    calendar.setAttribute('data-until', until);
    calendar.setAttribute('data-timestamp', start);
    calendar.setAttribute('data-select', this.editing.select);
    calendar.render(start);
  };

  this.renderDropdown = function() {
    var items = [];

    for (var key in this.selectableRanges) {
      var item = document.createElement("z-dropdown-item");
      item.setAttribute('data-value', key);
      item.innerHTML = this.selectableRanges[key];

      items.push(item);
    }

    this.querySelector("z-dropdown").setDropdownItems(items);
  };

  this.setFields = function(from, until) {
    var from_field = this.querySelector("input.from");
    var until_field = this.querySelector("input.until");
    var range = this.selectableRanges[until - from];

    from_field.value = this.getDateDescription(from);
    until_field.value = this.getDateDescription(until);

    if (range) {
      from_field.classList.remove('.active');
      until_field.classList.remove('.active');
    } else {
      //custom
      this.activateFields();
    }
  };

  this.setDateRange = function(from, until) {
    var range = new Date().getTime() - from ;
    var value = (range in this.selectableRanges) ?
      range : 'custom';

    this.querySelector("z-dropdown").setValue([value]);
  };

  this.getCalendarStart = function(from, until) {
    var start = DateUtil.getStartOfMonth(new Date().getTime(), -2);

    if (start < from) {
      return start;
    }

    return DateUtil.getStartOfMonth(until, -2);
  };

  this.getDateDescription = function(timestamp) {
    var date = new Date(timestamp);
    return (
      date.getFullYear() + "-" +
      DateUtil.appendLeadingZero(date.getMonth() + 1) + "-" +
      DateUtil.appendLeadingZero(date.getDate()));
  };

  this.toggleWidgetVisibility = function() {
    if (this.hasAttribute('data-active')) {
      this.removeAttribute('data-active');
    } else {
      this.setAttribute('data-active', 'active');
    }
  };

  this.highlightField = function() {
    this.querySelector("input.highlight").classList.remove('highlight');

    this.querySelector("input." + this.editing.select)
        .classList.add("highlight");
  };

  this.activateFields = function() {
    var inputs = this.querySelectorAll("input");

    for (var i = 0; i < inputs.length; i++) {
      inputs[i].classList.add('active');
    }
  };

  this.onCalendarSelect = function(timestamp) {
    var timestamp = DateUtil.getStartOfDay(timestamp);

    if (this.editing.select === 'from') {
      this.editing.select = 'until';
      this.editing.from = timestamp;
    } else {
      this.editing.select = 'from';
      this.editing.until = timestamp;
    }

    this.highlightField();
    this.activateFields();
    this.renderCalendar(this.editing.from, this.editing.until);
    this.setDateRange(this.editing.from, this.editing.until);
    this.setFields(this.editing.from, this.editing.until);
  };

  this.onFieldClick = function(range_value) {
    this.editing.select = range_value;
    this.activateFields();
    this.highlightField();
    this.setDateRange();

    this.renderCalendar(
      this.editing.from, this.editing.until,
      this.querySelector("z-calendar-group").firstDate);
  };

  this.onDropdownChange = function(range) {
    if (range == 'custom') {
      this.activateFields();
      return;
    }

    //today
    if (range == 0) {
      this.editing.until = Date.now();
      this.editing.from = DateUtil.getStartOfDay(this.editing.until);
    } else {
      this.editing.until = DateUtil.getStartOfDay(new Date().getTime());
      this.editing.from = this.editing.until - range;
    }

    this.setFields(this.editing.from, this.editing.until);
    this.renderCalendar(this.editing.from, this.editing.until);
  };

  this.fireSelectEvent = function() {
    var ev = new CustomEvent('select', {
      'detail' : {'from' : this.applied.from, 'until' : this.applied.until},
      'bubbles' : true,
      'cancelable' : true
    });

    this.dispatchEvent(ev);
  };
};

var proto = Object.create(HTMLElement.prototype);
DateRangePickerComponent.apply(proto);
document.registerElement("z-daterangepicker", { prototype: proto });

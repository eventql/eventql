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

    if (this.hasAttribute('data-resolved')) {
      this.init();
    }
  };

  this.attributeChangedCallback = function(attr, old_val, new_val) {
    if (attr == 'data-resolved') {
      this.init();
    }
  };

  this.init = function() {
    var _this = this;

    this.querySelector("z-daterangepicker-field").onclick = function(e) {
      e.stopPropagation();
      _this.toggleWidgetVisibility();
    };

    this.querySelector("z-daterangepicker-widget").onclick = function(e) {
      e.stopPropagation();
    };

    var from = DateUtil.getStartOfDay(
      DateUtil.parseTimestamp(this.getAttribute('data-from')));
    var until = DateUtil.getStartOfDay(
      DateUtil.parseTimestamp(this.getAttribute('data-until')));

    this.applied = {
      'from' : from,
      'until' : until
    };

    var _this = this;
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
      var target = e.target || e.srcElement;

      _this.onRangeDropdownSelect(target.getAttribute('data-value'));
    }, false);

    //set from
    this.querySelector("input.from").addEventListener("click", function() {
      _this.onRangeFieldClick("from");
    }, false);

    //set until
    this.querySelector("input.until").addEventListener("click", function() {
      _this.onRangeFieldClick("until");
    }, false);

    this.querySelector("z-calendar-group").addEventListener('select', function(e) {
        _this.onCalendarSelect(e.detail.timestamp);
    }, false);


    this.setWidget();
    this.renderPreview();
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
    calendar.render();
  };

  this.renderDropdown = function() {
    var items = this.querySelector("z-dropdown-items");

    for (var key in this.selectableRanges) {
      var item = document.createElement("z-dropdown-item");
      item.setAttribute('data-value', key);
      item.innerHTML = this.selectableRanges[key];

      items.appendChild(item);
    }

    this.querySelector("z-dropdown").setAttribute('data-resolved', 'resolved');
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
      this.brightenRangeFields();
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

};

var proto = Object.create(HTMLElement.prototype);
DateRangePickerComponent.apply(proto);
document.registerElement("z-daterangepicker", { prototype: proto });

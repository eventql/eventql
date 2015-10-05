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

    this.initWidget();
    this.renderPreviewField();

    this.observeRangeDropdown();
    this.observeCalendar();
    this.observeRangeFields();
    this.observeButtons();
  };

};

var proto = Object.create(HTMLElement.prototype);
DateRangePickerComponent.apply(proto);
document.registerElement("z-daterangepicker", { prototype: proto });

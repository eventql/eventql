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
var CalendarGroupComponent = function() {
  this.createdCallback = function() {
    var tpl = $.getTemplate(
        "widgets/z-calendar-group",
        "z-calendar-group-base-tpl");

    this.appendChild(tpl);
    this.render(this.getStartTimestamp());

    var _this = this;
    this.querySelector('.month_picker.prev').addEventListener('click', function() {
      _this.render(DateUtil.getStartOfMonth(_this.firstDate, -1));
    }, false);

    this.querySelector('.month_picker.next').addEventListener('click', function() {
      _this.render(DateUtil.getStartOfMonth(_this.firstDate, 1));
    }, false);
  };

  this.render = function(start_date) {
    var dates = [
      DateUtil.getStartOfMonth(start_date, 0),
      DateUtil.getStartOfMonth(start_date, 1),
      DateUtil.getStartOfMonth(start_date, 2)
    ];

    this.firstDate = dates[0];
    this.lastDate = dates[2];

    for (var i = 0; i < dates.length; i++) {
      this.renderMonth(i, dates[i]);
    }
  };

  this.renderMonthHeader = function(thead_elem, start_date) {
    var date = new Date(start_date);
    thead_elem.innerHTML =
      DateUtil.humanMonth[date.getMonth()] + " " + date.getFullYear();
  }

  this.renderMonth = function(num, start_date) {
    var table_elem = this.querySelectorAll("table.calendar")[num];
    var tr_elems = table_elem.querySelectorAll("tr.week");

    this.renderMonthHeader(table_elem.querySelector("thead tr td"), start_date);

    var current_date = this.renderFirstWeek(tr_elems[0], start_date);
    var num_days = DateUtil.getDaysInMonth(start_date);

    for (var i = 1; i < tr_elems.length; i++) {
      tr_elems[i].innerHTML = "";

      for (var day = 0; day < 7; day++) {
        //all days of current month are rendered
        if (current_date > num_days) {
          this.renderLastWeeks(day, i, tr_elems[i], num, start_date);
          return;
        }

        tr_elems[i].appendChild(
            this.renderDay(current_date, start_date, true));

        current_date++;
      }
    }
  };

  this.renderFirstWeek = function(tr_elem, start_date) {
    var preview = this.hasAttribute('data-preview');
    var prev_month = DateUtil.getStartOfMonth(start_date, -1);
    var prev_dates = this.datesOfPrevMonth(prev_month, start_date);
    var current_date = 1;

    tr_elem.innerHTML = '';

    //render last days of previous month
    for (var i = 0; i < prev_dates.length; i++) {
      tr_elem.appendChild(
        this.renderDay(prev_dates[i], prev_month, preview));
    };

    //render first days of current month
    for (day = prev_dates.length; day < 7; day ++) {
      tr_elem.appendChild(
        this.renderDay(current_date, start_date, true));

      current_date++;
    }

    return current_date;
  };

  this.renderLastWeeks = function() {
    return;
    //inidcates whether days of next months should be displayed or not
    var preview = this.hasAttribute('data-preview');

    var next_month = DateUtil.getStartOfMonth(date, 1);
    var current_date = 1;

    //add for remaining weekdays of week days of next month
    for (; day < 7; day++) {
      tr_elem.appendChild(
        this.renderDay(current_date, next_month, preview));

      current_date++;
    }

    //render week with days of next month
    for (week++; week < 6; week++) {
      var tr_elem = this.shadowRoot.getElementById(id + "_week" + week);
      tr_elem.innerHTML = "";

      for (day = 0; day < 7; day++) {
        tr_elem.appendChild(
          this.renderDay(current_date, next_month, preview));

        current_date++;
      }
    }
  };

  this.renderDay = function(date, month_timestamp, displayed) {
    var td_elem = document.createElement("td");

    if (displayed && displayed != "false") {
      var timestamp = month_timestamp + (date - 1) * DateUtil.millisPerDay;

      td_elem.className = this.getTdClassName(date, month_timestamp);

      if (this.isSelectableDate(date, month_timestamp)) {
        td_elem.classList.add("selectable");
        this.handleDateLinks(td_elem, date, month_timestamp);
      }

      td_elem.innerHTML = date;
    }

    return td_elem;
  };

  this.getTdClassName = function(date, month_timestamp) {
    var now = Date.now();
    var timestamp = month_timestamp + (date - 1) * DateUtil.millisPerDay;
    var name = "";

    // date isn't in current month
   /* if (month_timestamp !== this.date.month_timestamp) {
      name += " bright";
    }*/

    // date is today
    if (DateUtil.isSameDay(now, timestamp)) {
      name += " highlight_border";
    }

    if (this.hasAttribute('data-from') || this.hasAttribute('data-until')) {

      var from = this.hasAttribute('data-from') ?
        DateUtil.parseTimestamp(this.getAttribute('data-from')) : null;

      var until = this.hasAttribute('data-until') ?
        DateUtil.parseTimestamp(this.getAttribute('data-until')) : null;


      if ((from && until && timestamp >= from && timestamp <= until) ||
          (from && !until && timestamp >= from) ||
          (until && !from && timestamp <= until) ||
          DateUtil.isSameDay(from, timestamp) ||
          DateUtil.isSameDay(until, timestamp)) {
            name+= " colored";
      }

    }

    if (this.hasAttribute('data-selected')) {
      var selected_dates = [DateUtil.parseTimestamp(this.getAttribute('data-selected'))];

      for (var i = 0; i < selected_dates.length; i++) {
        // date is selected date
        if (DateUtil.isSameDay(selected_dates[i], timestamp)) {
          name += " highlight";
          break;
        }
      }
    }

    return name;
  };

  this.isSelectableDate = function(date, month_timestamp) {
    var now = Date.now();
    var timestamp = month_timestamp + (date - 1) * DateUtil.millisPerDay;
    var period = this.getAttribute('data-selectable');
    var select = this.getAttribute('data-select');
    var selectable = true;

    if (select == 'until') {
      var from = DateUtil.parseTimestamp(this.getAttribute('data-from'));
      selectable = timestamp >= from || DateUtil.isSameDay(from, timestamp);
    }

    if (period == "past") {
      selectable = selectable && timestamp < now;
    }

    if (period == "future") {
      selectable = selectable && timestamp > now;
    }

    return selectable;
  };

  this.getStartTimestamp = function() {
    var timestamp = (this.hasAttribute('data-timestamp')) ?
      this.getAttribute('data-timestamp') : this.getAttribute('data-from');

    return DateUtil.parseTimestamp(timestamp);
  };

      //timestamp -> beginning of last month
  this.datesOfPrevMonth = function(last_month, this_month) {
    var dates = [];

    var new_date = new Date(last_month);
    var num_days = DateUtil.daysInMonth(new_date.getMonth() + 1, new_date.getYear());
    var first_day = DateUtil.getFirstWeekdayOfMonth(this_month);

    for (var i = first_day - 1; i >= 0; i--) {
      dates.push(num_days - i);
    }

    return dates;
  };

  this.handleDateLinks = function(td_elem, date, month_timestamp) {
    var _this = this;
    var timestamp = month_timestamp + (date - 1) * DateUtil.millisPerDay;

    td_elem.onclick = function() {
      var ev = new CustomEvent('fn-calendar-group-select', {
        'detail' : {'timestamp' : timestamp},
        cancelable: true,
        bubbles: true
      });

      _this.dispatchEvent(ev);
    };
  };
};

var proto = Object.create(HTMLElement.prototype);
CalendarGroupComponent.apply(proto);
document.registerElement("z-calendar-group", { prototype: proto });

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
DateUtil = {};

//Time Constants
DateUtil.secondsPerMinute = 60;
DateUtil.secondsPerHour = 3600;
DateUtil.secondsPerDay = 86400;
DateUtil.millisPerSecond = 1000;
DateUtil.millisPerMinute = DateUtil.millisPerSecond * DateUtil.secondsPerMinute;
DateUtil.millisPerHour = DateUtil.secondsPerHour * DateUtil.millisPerSecond;
DateUtil.millisPerDay = DateUtil.secondsPerDay * DateUtil.millisPerSecond;
DateUtil.microsPerSecond = 1000000;
DateUtil.microsPerHour = 3600000000;

DateUtil.humanMonth =  [
  "January",
  "February",
  "March",
  "April",
  "May",
  "June",
  "July",
  "August",
  "September",
  "October",
  "November",
  "December"
];

/**
  * Returns the timestamp at midnight of the given timestamp
  *
  * @param {number} timestamp Milliseconds Timestamp
  */
DateUtil.getStartOfDay = function(timestamp) {
  return (timestamp - (timestamp % DateUtil.millisPerDay));
}


/**
  * Return timestamp at 00:00 at the first day of the month or x months later or earlier
  *
  * @param {number} timestamp
  * @offset {number} number of months earlier (< 0) or later (> 0)
  */
DateUtil.getStartOfMonth = function(timestamp, offset) {
  var timestamp = this.getStartOfDay(timestamp);
  var date = new Date(timestamp);

  //start of current month
  timestamp = timestamp - ((date.getDate() - 1)  * DateUtil.millisPerDay);

  if (offset) {

    for (var i = 1; i <= Math.abs(offset); i++) {
      if (offset < 0 ) {
        date = new Date(timestamp  - (timestamp % DateUtil.millisPerDay) - DateUtil.millisPerDay);
      } else {
        date = new Date(timestamp  - (timestamp % DateUtil.millisPerDay));
      }

      var daysInMonth = this.daysInMonth(date.getMonth() + 1, date.getFullYear());

      if (offset < 0) {
        timestamp = timestamp - (daysInMonth * DateUtil.millisPerDay);
      } else {
        timestamp = timestamp + (daysInMonth * DateUtil.millisPerDay);
      }
    }
  }

  return timestamp;
}


// @date DateObject as returned by DateUtil.getDateObject
DateUtil.getDateTimeDescr = function(date) {
  if (DateUtil.isNow(date.timestamp, date.precision)) {
    return "now";
  }


  var descr =
    date.year + "-" +
    DateUtil.appendLeadingZero(date.month + 1) + "-" +
    DateUtil.appendLeadingZero(date.date);

  if (date.precision == 'date') {
    return descr;
  }

  descr += " " + DateUtil.appendLeadingZero(date.hours);
  if (date.precision == 'hour') {
    return descr;
  }

  descr += ":" + DateUtil.appendLeadingZero(date.minutes);
  if (date.precision == 'minute') {
    return descr;
  }

  descr += ":" + DateUtil.appendLeadingZero(date.seconds);
  return descr;
};


//returns true if @timestamp is equal to the current timestamp (for the given @precision)
DateUtil.isNow = function(timestamp, precision) {
  return (DateUtil.equalDatesForPrecision(
    new Date().getTime(), timestamp, precision));
};

//returns true if @date is a JS Date Object and false otherwise
DateUtil.isInstanceOfDate = function(date) {
  return (date !== null && typeof date === 'object' && date instanceof Date)
};

DateUtil.getFirstWeekdayOfMonth = function(timestamp) {
  var first_day = new Date(DateUtil.getStartOfMonth(timestamp)).getDay();

  if (first_day == 0) {
    return 6;
  }

  return first_day - 1;
};



// @date Javascript Date instance or timestamp
DateUtil.getDateObject = function(date, precision, advanced) {
  if (!DateUtil.isInstanceOfDate(date)) {
    date = new Date(date);

    if (date == 'Invalid Date') {
      return;
    }
  }

  var dateObj = {
    'precision' : precision,
    'timestamp' : date.getTime(),
    'year': date.getFullYear(),
    'month': date.getMonth(),
    'date' : date.getDate(),
    'day' : date.getDate(),
  }

  if (advanced) {
    dateObj.num_days = DateUtil.daysInMonth(dateObj.month + 1, dateObj.year);

    var first_day = new Date(dateObj.year + "-" + (dateObj.month + 1) + "-01");

    //counting from 0 where Monday = 0 and Sunday = 6
    dateObj.first_day = (first_day.getDay() == 0) ? 6 : first_day.getDay() - 1;
    dateObj.month_timestamp = first_day.getTime();
  }

  if (precision == 'date') {
    return dateObj;
  }

  dateObj.hours = date.getHours()
  if (precision == 'hour') {
    return dateObj;
  }

  dateObj.minutes = date.getMinutes();
  if (precision == "minute") {
    return dateObj;
  }

  dateObj.seconds = date.getSeconds();
  if (precision == "second") {
    return dateObj;
  }

  return dateObj;
};

// compares two milliseconds timestamps and checks if they are equal for the
// given precision
DateUtil.equalDatesForPrecision = function(ts1, ts2, precision) {
  if (precision == "hour") {
    return (Math.ceil(ts1 / 360000) === Math.ceil(ts2 / 360000));
  }

  if (precision == "minute") {
    return (Math.ceil(ts1 / 60000) === Math.ceil(ts2 / 60000));
  }

  if (precision == "second") {
    return (Math.ceil(ts1 / 1000) === Math.ceil(ts2 / 1000));
  }

  return false;
};

// determines month and year if @offset != 0 and
// returns the timestamp for the first day of the month
DateUtil.getMonthTimestamp = function(base_month, base_year, offset) {
  var month = base_month + offset;
  var year = base_year;

  if (month == -1) {
    year = year - 1;
    month = 11;
  }

  if (month == 12) {
    year = year + 1;
    month = 0;
  }

  var d = new Date(year, month);
  return d.getTime() - d.getTimezoneOffset() * 60;
};

//checks if two timestamps are from the same day
DateUtil.isSameDay = function(ts1, ts2) {
  var day1 = new Date(ts1).setHours('0','0','0','0');
  var day2 = new Date(ts2).setHours('0','0','0','0');

  return (day1 === day2);
};

//returns a millisecond timestamp
DateUtil.parseTimestamp = function(timestamp) {
  //TODO add timezone
  var timestamp = parseInt(timestamp);
  //1st January 3000
  var thresholdSeconds = 32503680000;

  if (isNaN(timestamp)) {
    return Date.now();
  }

  if (timestamp < thresholdSeconds) {
    return timestamp * 1000;
  }

  return timestamp;
};

DateUtil.getDaysInMonth = function(timestamp) {
  var date = new Date(timestamp);

  return this.daysInMonth(date.getMonth() + 1, date.getYear());
}

DateUtil.daysInMonth = function(month, year) {
  if (month == 2) {
    return (28 + DateUtil.leapYearOffset());
  }

  return (31 - (month - 1) % 7 % 2);
};

DateUtil.leapYearOffset = function(year) {
  //year is leap year
  if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)) {
    return 1;
  }

  return 0;
};

//returns milliseconds since midnight for @timestamp
DateUtil.getTimeTimestamp = function(timestamp) {
  var date = new Date(timestamp);
  return (
    date.getHours() * DateUtil.millisPerHour +
    date.getMinutes() * DateUtil.millisPerMinute +
    date.getSeconds() * DateUtil.millisPerSecond);
};

DateUtil.getTimestampObj = function(timestamp, utc_offset) {
  if (utc_offset) {
    //difference between utc and local time
    var timezoneOffset = new Date().getTimezoneOffset();
    var utc_offset = parseInt(utc_offset);

    if (utc_offset != timezoneOffset) {
      var offset = (utc_offset - timezoneOffset) * DateUtil.millisPerMinute;
      //date.setTime(date.getTime() - offset);
      timestamp = timestamp - offset;
    }
  }

  var time_ts = DateUtil.getTimeTimestamp(timestamp);
  return {
    'time': time_ts,
    'date': timestamp - time_ts
  }
};

// @timestamp = milliseconds since midnight
DateUtil.getTimeObjFromTimestamp = function(timestamp) {
  var hours = Math.floor(timestamp / DateUtil.millisPerHour);
  timestamp = timestamp - (hours * DateUtil.millisPerHour);

  var minutes = Math.floor(timestamp / DateUtil.millisPerMinute);
  timestamp = timestamp - (minutes * DateUtil.millisPerMinute);

  return {
    'hours' : hours,
    'minutes' : minutes,
    'seconds' : Math.floor(timestamp / DateUtil.millisPerSecond)
  }
};

// returns seconds since midnight
DateUtil.getTimestampFromTimeObj = function(timeObj) {
  var timestamp = 0;
  if (timeObj.hours) {
    timestamp += timeObj.hours * DateUtil.millisPerHour;
  }

  if (timeObj.minutes) {
    timestamp += timeObj.minutes * DateUtil.millisPerMinute;
  }

  if (timeObj.seconds) {
    timestamp += timeObj.seconds * DateUtil.millisPerSecond;
  }

  return timestamp;
};

//FIXME implement different time zones
DateUtil.printTimestamp = function(ts, timezone) {
  var date = new Date(ts);
  return [
    date.getFullYear(), "-",
    this.appendLeadingZero(date.getMonth() + 1), "-",
    this.appendLeadingZero(date.getDate()), " ",
    this.appendLeadingZero(this.getHours(ts, timezone)), ":",
    this.appendLeadingZero(this.getMinutes(ts, timezone)), ":",
    this.appendLeadingZero(this.getSeconds(ts, timezone)), ".",
    this.appendLeadingZero(this.getMilliSeconds(ts, timezone))].join("");
};

DateUtil.appendLeadingZero = function (num) {
  var num = num;
  if (typeof num == 'string') {
    return (num.length > 1)? num : "0" + num;
  }
  return (num > 9)? num : "0" + num;
};


//FIXME implement different time zones
DateUtil.getHours = function(time, timezone) {
  var start_of_day = this.getStartOfDay(time);
  return Math.floor((time - start_of_day) / this.millisPerHour);
};

//FIXME implement different time zones
DateUtil.getMinutes = function(time, timezone) {
  time -= this.getStartOfDay(time);
  var hours = Math.floor(time / this.millisPerHour);
  time -= hours * this.millisPerHour;
  return Math.floor(time / this.millisPerMinute);
};

//FIXME implement different time zones
DateUtil.getSeconds = function(time, timezone) {
  time -= this.getStartOfDay(time);
  var hours = Math.floor(time / this.millisPerHour);
  time -= hours * this.millisPerHour;
  var minutes = Math.floor(time / this.millisPerMinute);
  time -= minutes * this.millisPerMinute;
  return Math.floor(time / this.millisPerSecond);
};

//FIXME implement different time zones
DateUtil.getMilliSeconds = function(time, timezone) {
  time -= this.getStartOfDay(time);
  var hours = Math.floor(time / this.millisPerHour);
  time -= hours * this.millisPerHour;
  var minutes = Math.floor(time / this.millisPerMinute);
  time -= minutes * this.millisPerMinute;
  var seconds = Math.floor(time / this.millisPerSecond);
  return time - seconds * this.millisPerSecond;
};

DateUtil.milliSecondsSinceMidnight = function(hours, minutes, seconds) {
  return (
      parseInt(hours, 10) * this.millisPerHour +
      parseInt(minutes, 10) * this.millisPerMinute +
      parseInt(seconds, 10) * this.millisPerSecond);
};

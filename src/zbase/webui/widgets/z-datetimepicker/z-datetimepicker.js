/**
  * require_module: "z-calendar"
  * require_module: "z-button"
 **/
function DateTimePicker(input) {
  var flyout;

  var init = function() {
    if (input.tagName == "Z-INPUT") {
      input = input.querySelector("input");
    }

    // no input provided
    if (input.tagName != "INPUT") {
      console.log("DateTimePicker Error: no input provided");
      return;
    }

    var tpl = $.getTemplate("widgets/z-datetimepicker", "z-datetimepicker-base-tpl");
    // insert tpl after input
    input.style.cursor = "pointer";
    input.parentNode.insertBefore(tpl, input.nextSibling);
    //FIXME
    flyout = input.nextElementSibling;

    // don't close on click within datetimepicker
    flyout.addEventListener("click", function(e) {
      e.stopPropagation();
    }, false);

    flyout.querySelector("button").addEventListener("click", function(e) {
      apply();
    }, false);

    handleVisibility();
    controlTimeInput();
  };

  var handleVisibility = function() {

    input.addEventListener("click", function(e) {
      e.stopPropagation();
      if (flyout.hasAttribute("data-active")) {
        hide();
      } else {
        show();
      }
    }, false);
  };


  var controlTimeInput = function() {
    var inputs = flyout.querySelectorAll("input");

    for (var i = 0; i < inputs.length; i++) {
      inputs[i].addEventListener("blur", function() {

        switch (this.value.length) {
          // if no value is given set it to 00
          case 0:
            this.value = "00";
            break;

          case 1:
            var value = parseInt(this.value, 10);
            // non-integer value
            if (isNaN(value)) {
              renderTimeControlError(this);
              return;
            }
            this.value = DateUtil.appendLeadingZero(value);
            break;

          case 2:
            // non-integer value
            if (isNaN(parseInt(this.value[0], 10)) ||
                isNaN(parseInt(this.value[1], 10))) {
              renderTimeControlError(this);
              return;
            }
            break;
        }

        var value = parseInt(this.value, 10);
        if (this.getAttribute("data-factor") == "3600") {
          // hours value > 24
          if (value > 23) {
            renderTimeControlError(this);
            return;
          }
        } else {
          // minutes or seconds value > 59
          if (value > 59) {
           renderTimeControlError(this);
            return;
          }
        }

        // correct input value
        removeTimeControlError(this);
      }, false);
    }
  };


  var removeTimeControlError = function(time_control) {
    time_control.classList.remove("error");

    // if only correct input values enable apply button
    if (flyout.querySelector("input.error") == null) {
      flyout.querySelector("button").removeAttribute("data-state");
    }
  };


  var renderTimeControlError = function(time_control) {
    time_control.classList.add("error");

    // disable apply button
    flyout.querySelector("button").setAttribute("data-state", "disabled");
  };


  var renderErrorMessage = function() {
    flyout.querySelector("z-datetimepicker-error")
      .setAttribute("data-active", "active");
  };


  var removeErrorMessage = function() {
    flyout.querySelector("z-datetimepicker-error")
        .removeAttribute("data-active", "active");
  };


  // set time value and human formatted time string
  var onTimeChange = function() {
    var timestamp = Math.floor(
      parseInt(input.getAttribute("data-timestamp"), 10));

    // set input value as human formatted string
    input.value = DateUtil.printTimestamp(timestamp);

    timestamp = Math.floor(timestamp / 1000);
    var date = new Date(timestamp);
    var utc_day_start = DateUtil.getStartOfDay(timestamp) +
      date.getTimezoneOffset() * DateUtil.millisPerMinute;

    // set z-calendar selection
    flyout.querySelector("z-calendar").setAttribute("data-selected", utc_day_start);
    // set hours input value
    flyout.querySelector("input[data-value='hours']").value =
      DateUtil.appendLeadingZero(date.getHours());
    // set minutes input value
    flyout.querySelector("input[data-value='minutes']").value =
      DateUtil.appendLeadingZero(date.getMinutes());
    // set seconds input value
    flyout.querySelector("input[data-value='seconds']").value =
      DateUtil.appendLeadingZero(date.getSeconds());
  };


  // get selected datetime as timestamp
  var getTimeValue = function() {
    var timestamp = DateUtil.parseTimestamp(
      parseInt(flyout.querySelector("z-calendar")
        .getAttribute("data-selected"), 10));

    var inputs = flyout.querySelectorAll("input");
    for (var i = 0; i < inputs.length; i++) {
      var value = parseInt(inputs[i].value, 10);
      timestamp += value * parseInt(inputs[i].getAttribute("data-factor"), 10);
    }

    timestamp = timestamp * 1000;
    return timestamp;
  };


  // checks if the selected date is in the past
  var isValidTimeValue = function(timestamp) {
    // error -> future datetime
    if (Math.floor(timestamp / 1000) > Date.now()) {
      renderErrorMessage();
      return false;
    }

    removeErrorMessage();
    return true;
  };


  var apply = function() {
    var timestamp = getTimeValue();

    // invalid timestamp
    if (!isValidTimeValue(timestamp)) {
      return;
    }

    input.setAttribute("data-timestamp", timestamp);
    onTimeChange();
    hide();

    // fire change event on input
    var evt = new CustomEvent("z-datetimepicker-change");
    input.dispatchEvent(evt);
  };


  var hide = function() {
    flyout.removeAttribute("data-active");

    window.removeEventListener("click", onWindowClick, false);
  };


  var show = function() {
    onTimeChange();
    var pos = input.getBoundingClientRect();

    // set flyout top and left position
    flyout.style.top = (pos.top + pos.height) + "px";
    flyout.setAttribute("data-active", "active");
    flyout.style.left = (pos.left - (flyout.offsetWidth - pos.width) / 2) + "px";


    window.addEventListener("click", onWindowClick, false);
  };

  var onWindowClick = function() {
    hide();
  };


  init();
  return {
    show: show,
    hide: hide
  };
};

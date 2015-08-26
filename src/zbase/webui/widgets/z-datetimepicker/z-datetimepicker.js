/**
  * require_module: "z-calendar"
  * require_module: "z-button"
 **/
function DateTimePicker(input) {

  var getTimezone = function() {
    var tz = input.getAttribute("data-timezone");
    if (!tz) tz = "UTC";
    return tz;
  }

  var getTime = function() {
    return Math.floor(parseInt(input.getAttribute("data-timestamp"), 10) / 1000);
  }

  var setTime = function(new_time) {
    if (!new_time) {
      new_time = getTime();
    }

    input.setAttribute("data-timestamp", new_time);
    input.value = DateUtil.printTimestamp(new_time);
  }

  var toggleFlyout = function() {
    if (flyout.hasAttribute("data-active")) {
      hideFlyout();
    } else {
      showFlyout();
    }
  };

  var hideFlyout = function() {
    window.removeEventListener("click", hideFlyout, false);
    flyout.removeAttribute("data-active");
  };

  var showFlyout = function() {
    var time = getTime();
    var tz = getTimezone();

    $("z-calendar").setAttribute("data-selected", time);
    $(".hours_control", flyout).value = DateUtil.getHours(time, tz);
    $(".minutes_control", flyout).value = DateUtil.getMinutes(time, tz);
    $(".seconds_control", flyout).value = DateUtil.getSeconds(time, tz);

    var pos = input.getBoundingClientRect();
    flyout.setAttribute("data-active", "active");
    flyout.style.top = (pos.top + pos.height + window.scrollY) + "px";
    flyout.style.left = (pos.left - (flyout.offsetWidth - pos.width) / 2) + "px";

    window.addEventListener("click", hideFlyout, false);
  };

  var submit = function() {
    console.log("update value...");
    //var new_time = DateUtil.fromCivilTime(
        // year,
        // month,
        // day,
        // hours,
        // minutes,
        // seconds);

    setTime(new_time);
    hideFlyout();
  };


//
//
//  // get selected datetime as timestamp
//  var getTimeValue = function() {
//    var timestamp = DateUtil.parseTimestamp(
//      parseInt(flyout.querySelector("z-calendar")
//        .getAttribute("data-selected"), 10));
//
//    var inputs = flyout.querySelectorAll("input");
//    for (var i = 0; i < inputs.length; i++) {
//      var value = parseInt(inputs[i].value, 10);
//      timestamp += value * parseInt(inputs[i].getAttribute("data-factor"), 10);
//    }
//
//    timestamp = timestamp * 1000;
//    return timestamp;
//  };

//
//  var apply = function() {
//    var timestamp = getTimeValue();
//
//    // invalid timestamp
//    if (!isValidTimeValue(timestamp)) {
//      return;
//    }
//
//    input.setAttribute("data-timestamp", timestamp);
//    onTimeChange();
//    hide();
//
//    // fire change event on input
//    var evt = new CustomEvent("z-datetimepicker-change");
//    input.dispatchEvent(evt);
//  };

  if (input.tagName == "Z-INPUT") {
    input = input.querySelector("input");
  }

  if (input.tagName != "INPUT") {
    throw("DateTimePicker Error: no input provided");
  }

  var tpl = $.getTemplate(
      "widgets/z-datetimepicker",
      "z-datetimepicker-base-tpl");

  var flyout = $("z-datetimepicker-flyout", tpl);
  $.stopEventPropagation(flyout, "click");
  $.onClick($("button", flyout), submit);
  $.onClick(input, toggleFlyout);
  input.parentNode.insertBefore(tpl, input.nextSibling);
  setTime();

  return {
    show: showFlyout,
    hide: hideFlyout,
    getTime: getTime,
    setTime: setTime
  };
};

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
    return parseInt(input.getAttribute("data-timestamp"), 10);
  }

  var setTime = function(new_time) {
    if (!new_time) {
      new_time = getTime();
    }

    input.setAttribute("data-timestamp", new_time);
    input.value = DateUtil.printTimestamp(new_time);
    console.log("set tiem");
  };

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
    var date = DateUtil.getStartOfDay(
        $("z-calendar", flyout).getAttribute("data-selected"));

    var time = DateUtil.milliSecondsSinceMidnight(
        $(".hours_control", flyout).value,
        $(".minutes_control", flyout).value,
        $(".seconds_control", flyout).value);

    setTime(date + time);
    hideFlyout();

    var evt = new CustomEvent("z-datetimepicker-change");
    input.dispatchEvent(evt);
  };


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

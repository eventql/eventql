var JourneyDetail = function() {
  var render = function(page, data) {
    var tpl = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_journey_detail_tpl");

    $(".session_id", tpl).innerHTML = data.event.session_id;
    $(".time", tpl).innerHTML =
        DateUtil.printTimestampShort(data.event.first_seen_time / 1000) +
        "&mdash;" +
        DateUtil.printTimestampShort(data.event.last_seen_time / 1000);

    var attribute_pane = $(".attributes", tpl);
    for (var attr in data.event.attr) {
      var html =
          "<div><label>" + attr + ":</label><span>" + data.event.attr[attr] + "</span></div>";
      attribute_pane.innerHTML += html;
    }

    var events_pane = $(".events", tpl);
    for (var ev in data.event.event) {
      renderEvent(ev, data.event.event[ev], events_pane);
    }

    $.onClick($("button.back", tpl), function() {
      $.navigateTo("/a/session_tracking/journey_viewer");
    });
    $.replaceContent(page, tpl);
  };

  var renderEvent = function(name, json, elem) {
    console.log(name, json);
    var tpl = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_journey_detail_event_tpl");

    $("h3", tpl).innerHTML = name;

    elem.appendChild(tpl);
  };

  return {
    render: render
  }
}

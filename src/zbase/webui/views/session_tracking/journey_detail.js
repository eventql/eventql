var JourneyDetail = function() {
  var render = function(page, data) {
    var tpl = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_journey_detail_tpl");

    $(".session_id", tpl).innerHTML = data.event.session_id;
    $(".session_json", tpl).innerHTML = prettyPrintJSON(JSON.stringify(data));

    $.onClick($("button.back", tpl), function() {
      $.navigateTo("/a/session_tracking/journey_viewer");
    });
    $.replaceContent(page, tpl);
  };

  var prettyPrintJSON = function(json_str) {
    var pretty_json = "";
    var space = 0;

    for (var i = 0; i < json_str.length; i++) {
      if (json_str[i] == "[") {
        space++;
        pretty_json += "[ <br />";
        for (var j = 0; j < space; j++) {
          pretty_json += "&nbsp;&nbsp;&nbsp;"
        }
        continue;
      }
      if (json_str[i] == "{") {
        space++;
        pretty_json += "{ <br />";
        for (var j = 0; j < space; j++) {
          pretty_json += "&nbsp;&nbsp;&nbsp;"
        }
        continue;
      }
      if (json_str[i] == ",") {
        pretty_json += ", <br />";
        for (var j = 0; j < space; j++) {
          pretty_json += "&nbsp;&nbsp;&nbsp;"
        }
        continue;
      }

      if (json_str[i] == "]") {
        space--;
        pretty_json += "<br />";
        for (var j = 0; j < space; j++) {
          pretty_json += "&nbsp;&nbsp;&nbsp;"
        }
        pretty_json += "]";
        continue;
      }
      if (json_str[i] == "}" ) {
        space--;
        pretty_json += "<br />";
        for (var j = 0; j < space; j++) {
          pretty_json += "&nbsp;&nbsp;&nbsp;"
        }
        pretty_json += "}";
        continue;
      }
      pretty_json += json_str[i];
    }

    return pretty_json;
  };

  /*var renderEvent = function(name, events, elem) {
    var tpl = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_journey_detail_event_tpl");

    $("h3", tpl).innerHTML = name;
    var event_data = $(".event_data", tpl);

    for (var i = 0; i < events.length; i++) {
      var html = document.createElement("div");
      html.innerHTML = JSON.stringify(events[i]);
      if (i < events.length - 1) {
        html.innerHTML += ",";
      }
      event_data.appendChild(html);
    }

    elem.appendChild(tpl);
  };*/

  return {
    render: render
  }
}

ZBase.registerView((function() {
  var load = function(path) {
    $.showLoader();

    var page = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_main_tpl");

    var content = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_attributes_tpl");

    var menu = SessionTrackingMenu(path);
    menu.render($(".zbase_content_pane .session_tracking_sidebar", page));

    $(".zbase_content_pane .session_tracking_content", page).appendChild(content);

    loadAttributes();

    $.onClick($(".add_session_attribute .link", page), function() {
      renderAddAttributePane();
    });

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var loadAttributes = function() {
    $.httpGet("/api/v1/session_tracking/attributes", function(r) {
      if(r.status == 200) {
        renderAttributes(JSON.parse(r.response).session_attributes);
        //$.handleLinks(page); //call?
      } else {
        $.fatalError();
      }
      $.hideLoader();
    });
  };

  var renderAttributes = function(attributes) {
    var tbody = $(".zbase_settings table.attributes tbody");
    var tpl = $.getTemplate(
      "views/session_tracking",
      "zbase_session_tracking_attribute_row_tpl");

    tbody.innerHTML = "";
    attributes.columns.forEach(function(attr) {
      var html = tpl.cloneNode(true);
      $(".attribute_name", html).innerHTML = attr.name;
      $(".attribute_type", html).innerHTML = attr.type;

      $.onClick($(".icon", html), function() {
        alert("not yet implemented");
      });
      tbody.appendChild(html);
    });
  };

  var renderAddAttributePane = function() {
    var pane = $("table.add_attribute");
    var tpl = $.getTemplate(
        "views/session_tracking",
        "zbase_session_tracking_add_attribute_tpl");

    $("tr td", tpl).style.width = $("table.attributes tr td").offsetWidth + "px";
    pane.innerHTML = "";
    pane.appendChild(tpl);

    pane.classList.remove("hidden");
    $(".add_session_attribute").classList.add("hidden");

    $.onClick($("button[data-action='add-attribute']", pane), function() {
      addAttribute();
    });
  };

  var addAttribute = function() {
    var attribute = {};
    attribute.name = $.escapeHTML($("table.add_attribute input").value);

    if (attribute.name.length == 0) {
      $("table.add_attribute .error_note").classList.remove("hidden");
      return;
    }

    attribute.type= $("table.add_attribute z-dropdown").getValue();
    alert("POST new attribute");
    $.showLoader();
    hideAddAttributePane();
    //move into post
    loadAttributes();
    $.hideLoader();
  }

  var hideAddAttributePane = function() {
    $(".add_session_attribute").classList.remove("hidden");
    $("table.add_attribute").classList.add("hidden");
  };

  return {
    name: "session_tracking_attributes",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());


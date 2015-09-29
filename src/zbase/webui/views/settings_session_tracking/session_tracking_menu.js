var SessionTrackingMenu = function() {
  var render = function(elem) {
    var tpl = $.getTemplate(
        "views/settings_session_tracking",
        "zbase_session_tracking_menu_tpl");

    setActiveMenuItem(tpl);

    elem.appendChild(tpl);
  };

  var setActiveMenuItem = function(menu) {

  };

  return {
    render: render
  }
};

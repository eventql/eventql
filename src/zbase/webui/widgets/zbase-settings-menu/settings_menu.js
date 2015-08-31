var SettingsMenu = function() {

  var render = function(elem) {
    var tpl = $.getTemplate(
        "widgets/zbase-settings-menu",
        "zbase_settings_menu_main_tpl");

    elem.innerHTML = "";
    elem.appendChild(tpl);
  };

  return {
    render: render
  };

};


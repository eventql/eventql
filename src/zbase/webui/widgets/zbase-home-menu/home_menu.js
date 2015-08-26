var HomeMenu = function() {

  var render = function(elem) {
    var tpl = $.getTemplate(
        "widgets/zbase-home-menu",
        "zbase_home_menu_main_tpl");

    elem.innerHTML = "";
    elem.appendChild(tpl)
  };

  return {
    render: render
  };

};

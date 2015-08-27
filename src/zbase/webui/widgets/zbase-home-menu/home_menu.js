var HomeMenu = function() {

  var render = function(elem) {
    var tpl = $.getTemplate(
        "widgets/zbase-home-menu",
        "zbase_home_menu_main_tpl");

    setActiveMenuItem(tpl);

    elem.innerHTML = "";
    elem.appendChild(tpl);
  };

  var setActiveMenuItem = function(tpl) {
    var active_item = $("z-menu-item[data-active]", tpl);
    if (active_item) {
      active_item.removeAttribute("data-active");
    }

    var new_active_link = $(
        "z-menu-item a[href='" + window.location.pathname + "']",
        tpl);
    if (new_active_link) {
      new_active_link.parentNode.setAttribute("data-active", "active");
    }
  };

  return {
    render: render
  };

};

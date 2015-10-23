var ZBaseMainMenu = function() {
  var render = function(elem, path) {
    var tpl = $.getTemplate(
        "widgets/zbase-main-menu",
        "zbase_main_menu_tpl");

    setActiveMenuItem(tpl, "/a/sql");

    elem.appendChild(tpl);
  };

  var setActiveMenuItem = function(tpl, path) {
    var items = tpl.querySelectorAll("a");
    var active_path_length = 0;
    var active_item;

    for (var i = 0; i < items.length; i++) {
      var current_path = items[i].getAttribute("href");
      if (path.indexOf(current_path) == 0 &&
          current_path.length > active_path_length) {
              active_item = items[i];
              active_path_length = current_path.length;
      }
    }

    active_item.setAttribute("data-active", "active");
  };

  return {
    render: render
  }
};

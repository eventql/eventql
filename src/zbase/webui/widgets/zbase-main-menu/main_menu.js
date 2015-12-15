var ZBaseMainMenu = (function() {
  var render = function(path) {
    var tpl = $.getTemplate(
        "widgets/zbase-main-menu",
        "zbase_main_menu_tpl");

    var menu = document.getElementById("zbase_main_menu");

    $.replaceContent(menu, tpl);
    $.handleLinks(menu);
    setActiveMenuItem(path);

    var toggler = document.getElementById("menu_toggler");
    if (toggler) {
      $.onClick(toggler, function() {
        menu.classList.toggle("hidden");
        toggler.classList.toggle("closed");
      });
    }
  };

  var hideMenu = function() {
    var menu = document.getElementById("zbase_main_menu");
    if (menu) {
      menu.classList.add("hidden");
    }

    var toggler = document.getElementById("menu_toggler");
    if (toggler) {
      toggler.classList.add("closed");
    }
  };

  var showMenu = function() {
    var menu = document.getElementById("zbase_main_menu");
    if (menu) {
      menu.classList.remove("hidden");
    }

    var toggler = document.getElementById("menu_toggler");
    if (toggler) {
      toggler.classList.remove("closed");
    }
  };

  var setActiveMenuItem = function(path) {
    var menu = document.getElementById("zbase_main_menu");
    var current_active = $("a[data-active]", menu);
    if (current_active) {
      current_active.removeAttribute("data-active");
    }

    var items = menu.querySelectorAll("a");
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

    if (active_item) {
      active_item.setAttribute("data-active", "active");
    }
  };

  return {
    render: render,
    update: setActiveMenuItem,
    hide: hideMenu,
    show: showMenu
  }
})();

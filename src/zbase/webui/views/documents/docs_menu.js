var DocsMenu = function(categories) {

  var render = function(elem) {
    var tpl = $.getTemplate(
        "views/documents",
        "zbase_documents_menu_main_tpl");

    var menu = $("z-menu", tpl);
    categories.forEach(function(c) {
      insertMenuItem(c.split("~"), menu);
    });

    setActiveMenuItem(tpl);

    elem.innerHTML = "";
    elem.appendChild(tpl);
  };

  var setActiveMenuItem = function(tpl) {
    var path = window.location.pathname;
    var items = tpl.querySelectorAll("z-menu-item a[href]");
    var active_path_length = 0;
    var active_item;

    //longest prefix match
    for (var i = 0; i < items.length; i++) {
      var current_path = items[i].getAttribute("href");
      if (path.indexOf(current_path) == 0 &&
          current_path.length > active_path_length) {
              active_item = items[i];
              active_path_length = current_path.length;
      }
    }

    active_item.parentNode.setAttribute("data-active", "active");
  };

  var insertMenuItem = function(path, elem) {
    if (path.length == 0) {
      return;
    }

    if (path.length == 1) {
      var item_title = path[0];
      var item = elem.querySelector(
          "z-menu-item[data-key='" + item_title + "']"); // FIXME escaping

      if (!item) {
        item = document.createElement("z-menu-item");
        item.setAttribute("data-key", item_title);
        elem.appendChild(item);

        var link = document.createElement("a");
        link.innerHTML = $.escapeHTML(item_title);
        link.href = "#";
        item.appendChild(link);
      }
    } else {
      var section_title = path.shift();
      var section = elem.querySelector(
          "z-menu-section[data-key='" + section_title + "']"); // FIXME escaping

      if (!section) {
        section = document.createElement("z-menu-section");
        section.setAttribute("data-key", section_title);
        elem.appendChild(section);

        var title = document.createElement("z-menu-title");
        title.innerHTML = $.escapeHTML(section_title);
        section.appendChild(title);
      }

      insertMenuItem(path, section);
    }
  }

  return {
    render: render
  };

};

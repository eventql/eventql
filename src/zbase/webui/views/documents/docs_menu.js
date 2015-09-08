var DocsMenu = function(categories, num_documents) {
  var menu;

  var render = function(elem) {
    var tpl = $.getTemplate(
        "views/documents",
        "zbase_documents_menu_main_tpl");

    menu = $("z-menu", tpl);
    $(".num_documents", menu).innerHTML = toHumanNumber(num_documents);
    categories.forEach(function(c) {
      insertMenuItem([], c.split("~"), menu);
    });

    $.handleLinks(elem);
    elem.innerHTML = "";
    elem.appendChild(tpl);
  };

  var setActiveMenuItem = function(raw_key) {
    var key = encodeURIComponent(raw_key);
    if (!menu) {
      return;
    }

    var items = menu.querySelectorAll("z-menu-item, z-menu-section");
    if (!items) {
      return;
    }

    for (var i = 0; i < items.length; ++i) {
      var e = items[i];

      if (key.indexOf(e.getAttribute("data-key")) == 0) {
        e.setAttribute("data-active", "active");
      } else {
        e.removeAttribute("data-active");
      }
    }
  };

  var insertMenuItem = function(path, tail, elem) {
    if (tail.length == 0) {
      return;
    }

    path.push(tail[0]);
    var key = encodeURIComponent(path.join("~"));
    var href = "/a/?" + $.buildQueryString({
      publishing_status: "PUBSTATUS_PUBLISHED",
      category: path.join("~")
    });

    // insert section
    if (path.length == 1) {
      var section_title = tail.shift();
      var section = elem.querySelector(
          "z-menu-section[data-key='" + key + "']");

      if (!section) {
        section = document.createElement("z-menu-section");
        section.setAttribute("data-key", key);
        elem.appendChild(section);

        var title = document.createElement("z-menu-title");
        section.appendChild(title);

        var link = document.createElement("a");
        link.innerHTML = $.escapeHTML(section_title);
        link.href = href;
        title.appendChild(link);
      }

      insertMenuItem(path, tail, section);
    } else {
      var item_title = tail.shift();
      var item = elem.querySelector("z-menu-item[data-key='" + key + "']");
      if (!item) {
        item = document.createElement("z-menu-item");
        item.setAttribute("data-key", key);
        $.onClick(item, function() { ZBase.navigateTo(href); });
        elem.appendChild(item);

        var link = document.createElement("a");
        link.innerHTML = $.escapeHTML(item_title);
        link.href = href;
        item.appendChild(link);
      }

      insertMenuItem(path, tail, item);
    }
  };

  var toHumanNumber = function(number) {
    if (number < 1000) {
      return number;
    }

    var number_str = number.toString();
    var hundred_tail = number_str.substr(number_str.length - 3);
    var head = number_str.substr(0, number_str.length - 3);
    return head + "," + hundred_tail;
  };

  return {
    render: render,
    setActiveMenuItem: setActiveMenuItem
  };

};

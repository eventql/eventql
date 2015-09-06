var DocsMenu = function() {

  var render = function(elem) {
    var tpl = $.getTemplate(
        "views/documents",
        "zbase_documents_menu_main_tpl");

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

  return {
    render: render
  };

};

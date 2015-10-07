var DocumentationMenu = function() {

  var createMenuItems = function(parent, items, index, current) {
    var j = 0;

    for(var i in items) {
      var item = i;
      var key = item.toLowerCase().replace(/[ /]/g, "_");

      var section = document.createElement("z-menu-item");
      section.classList.add("link", "doc_section");
      parent.appendChild(section);

      var link = document.createElement("a");
      if(key == current) link.classList.add("current");
      link.href = "/a/documentation/" + key;
      var indexSpan = document.createElement("span");
      var textSpan = document.createElement("span");
      indexSpan.classList.add("index");
      indexSpan.innerHTML = index.concat(++j).join(".") + ".";
      textSpan.innerHTML = item;
      link.appendChild(indexSpan);
      link.appendChild(textSpan);
      section.appendChild(link);

      if(Object.keys(items[i]).length > 0) {
        createMenuItems(section, items[i], index.concat(+j), current);
      }
    }
  };

  var render = function(elem, key) {
    var tpl = $.getTemplate("widgets/zbase-documentation-menu", "zbase_documentation_menu_main_tpl");

    elem.innerHTML = "";
    elem.appendChild(tpl);

    $.httpGet("/a/_/d/toc.json", function(res) {
      var docStructure = JSON.parse(res.response);

      var menuSection = $("z-menu-section", elem);

      createMenuItems(menuSection, docStructure.toc, [], key);

      $.handleLinks(menuSection);
    });
  };


  return {
    render: render
  };

};

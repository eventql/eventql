var DocumentationMenu = function() {
  var tempJson = [
    {
      key: "format",
      title: "Format",
      chapters: [
        {
          key: "output",
          title: "Output",
          file: "output.html"
        },
        {
          key: "readme_and_introduction",
          title: "Readme and Introduction",
          file: "readme_and_introduction.html"
        }
      ]
    },
    {
      key: "build",
      title: "Build",
      chapters: [
        {
          key: "update_with_git",
          title: "Update with Git",
          file: "update_with_git.html"
        }
      ]
    }
  ];

  // Recursively add z-menu-items to the sidebar using tempJson.
  var createMenuItems = function(parent, items, index) {

    for(var i in items) {
      var item = items[i];

      var section = document.createElement("z-menu-item");
      section.classList.add("link", "doc_section");
      parent.appendChild(section);

      var link = document.createElement("a");
      link.href = "/a/documentation/" + item.key;
      var indexSpan = document.createElement("span");
      var textSpan = document.createElement("span");
      indexSpan.classList.add("index");
      indexSpan.innerHTML = index.concat(+i + 1).join(".");
      textSpan.innerHTML = item.title;
      link.appendChild(indexSpan);
      link.appendChild(textSpan);
      section.appendChild(link);

      if(item.chapters) {
        createMenuItems(section, item.chapters, index.concat(+i + 1));
      }
    }
  };

  var render = function(elem) {
    var tpl = $.getTemplate("widgets/zbase-documentation-menu", "zbase_documentation_menu_main_tpl");

    elem.innerHTML = "";
    elem.appendChild(tpl);

    $.httpGet("/a/_/d/toc.json", function(res) {
      //var docStructure = JSON.parse(res.response);
      var docStructure = tempJson;

      var menuSection = $("z-menu-section", elem);

      createMenuItems(menuSection, docStructure, []);

      $.handleLinks(menuSection);
    });
  };


  return {
    render: render
  };

};

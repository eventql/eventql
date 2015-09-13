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
    console.log(item);

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
    setActiveMenuItem(tpl);

    elem.innerHTML = "";
    elem.appendChild(tpl);

    $.httpGet("/a/_/d/toc.json", function(res) {
      //var docStructure = JSON.parse(res.response);
      var docStructure = tempJson;
      console.log(res.response);

      createMenuItems($("z-menu-section", elem), docStructure, []);
    });
  };

  var setActiveMenuItem = function(tpl) {
    /*
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
    */
  };


  return {
    render: render
  };

};

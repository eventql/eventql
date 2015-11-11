var DocumentationMenu = function() {

  var createMenuItems = function(parent, items, index, current_path) {
    var j = 0;
    var path_prefix = "/docs/";

    items.forEach(function(item) {
      var path = item.path;

      var section = document.createElement("z-menu-item");
      section.classList.add("link", "doc_section");
      parent.appendChild(section);

      var link = document.createElement("a");
      if (path_prefix + path == current_path) link.classList.add("active");
      link.href = "/docs/" + path;
      var indexSpan = document.createElement("span");
      var textSpan = document.createElement("span");
      indexSpan.classList.add("index");
      indexSpan.innerHTML = index.concat(++j).join(".") + ".";
      textSpan.innerHTML = item.title;
      link.appendChild(indexSpan);
      link.appendChild(textSpan);
      section.appendChild(link);

      if (item.children) {
        createMenuItems(section, item.children, index.concat(+j), current_path);
      }
    });
  };

  var render = function(elem, path) {
    httpGet("/docs/_/d/toc.json", function(res) {
      var docStructure = JSON.parse(res.response);
      createMenuItems(elem, docStructure.toc, [], path);
    });
  };

  var httpGet = function(url, callback) {
    var http = new XMLHttpRequest();
    http.open("GET", url, true);
    http.send();

    var base = this;
    http.onreadystatechange = function() {
      if (http.readyState == 4) {
        callback(http);
      }
    }
  };


  return {
    render: render
  };

};

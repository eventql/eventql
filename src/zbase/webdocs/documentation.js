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
    httpGet("/docs/_/d/toc.json", function(res) {
      var docStructure = JSON.parse(res.response);
      createMenuItems(elem, docStructure.toc, [], key);
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

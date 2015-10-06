ZBase.registerView((function() {

  var render = function(path) {
    if(path) var key = path.split("/").pop();

    var page = $.getTemplate(
        "views/documentation",
        "documentation_main_tpl");

    var menu = DocumentationMenu();
    menu.render($(".zbase_documentation_menu_sidebar", page), key);
    $.handleLinks(page);
    $.replaceViewport(page);

    var content = $(".zbase_documentation_content");

    if(path && path.split("/").length > 3) {
      $.httpGet("/a/_/d/" + key + ".html", function(res) {
        if(res.status == 200) {
          content.innerHTML = res.response;
        } else {
          //$.fatalError("Could not get markdown file \"" + key + ".html\".");
          content.innerHTML = key;
        }
      });
    }

    $.hideLoader();
  };

  return {
    name: "documentation",
    loadView: function(params) { render(); },
    unloadView: function() {},
    handleNavigationChange: render
  };

})());

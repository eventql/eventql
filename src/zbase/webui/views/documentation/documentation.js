ZBase.registerView((function() {

  var render = function() {
    var page = $.getTemplate(
        "views/documentation",
        "documentation_main_tpl");

    var menu = DocumentationMenu();
    menu.render($(".zbase_documentation_menu_sidebar", page));

    $.handleLinks(page);
    $.replaceViewport(page);

    $.hideLoader();
  };

  return {
    name: "documentation",
    loadView: function(params) { render(); },
    unloadView: function() {},
    handleNavigationChange: render
  };

})());

ZBase.registerView((function() {

  var render = function() {
    var page = $.getTemplate(
        "views/documentation",
        "documentation_main_tpl");

    var menu = DocumentationMenu();
    menu.render($(".zbase_documentation_menu_sidebar", page));
    $.handleLinks(page);
    $.replaceViewport(page);

    var content = $(".zbase_documentation_content");
    var example = $.getTemplate(
        "views/documentation",
        "documentation_page_examples");
        console.log(content);
    content.appendChild(example);

    var path = window.location.pathname;

    $.hideLoader();
  };

  return {
    name: "documentation",
    loadView: function(params) { render(); },
    unloadView: function() {},
    handleNavigationChange: render
  };

})());

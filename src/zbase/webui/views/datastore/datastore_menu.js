var DatastoreMenu = function() {

  var render = function(elem) {
    var tpl = $.getTemplate(
        "views/datastore",
        "zbase_datastore_menu_main_tpl");

    elem.innerHTML = "";
    elem.appendChild(tpl)
  };

  return {
    render: render
  };

};

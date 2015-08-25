ZBase.registerView((function() {

  var load = function(url) {
    $.showLoader();

    $.httpGet("/api/v1/logfiles", function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response).logfile_definitions);
      } else {
        $.fatalError();
      }

      $.hideLoader();
    });
  }

  var render = function(logfiles) {
    var layout = $.getTemplate("views/datastore", "zbase_datastore_main_tpl");

    var menu = DatastoreMenu();
    menu.render($(".datastore_sidebar", layout));

    var page = $.getTemplate(
        "views/datastore_logfiles",
        "zbase_datastore_logfiles_list_tpl");

    var tbody = $("tbody", page);
    logfiles.forEach(function(def) {
      var url = "/a/logviewer/" + def.name;
      var tr = document.createElement("tr");
      tr.innerHTML = 
          "<td><a href='" + url + "'>" + def.name + "</a></td>" +
          "<td><a href='" + url + "'>&mdash;</a></td>" +
          "<td><a href='" + url + "'>&mdash;</a></td>";

      $.onClick(tr, function() { $.navigateTo(url); });
      tbody.appendChild(tr);
    });

    $.replaceContent($(".datastore_viewport", layout), page);
    $.handleLinks(layout);
    $.replaceViewport(layout);
  };

  return {
    name: "datastore_logfiles",
    loadView: function(params) { load(params.url); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

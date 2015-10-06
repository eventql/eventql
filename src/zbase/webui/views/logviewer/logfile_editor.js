ZBase.registerView((function() {

  var load = function(path) {
    var path_prefix = "/a/logs/";
    var logfile = path.substr(path_prefix.length);

    $.httpGet("/api/v1/logfiles/get_definition?logfile=" + logfile, function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response));
      } else {
        $.fatalError();
      }
    });
  };

  var render = function(def) {
    var page = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_tpl");

    $("h1", page).innerHTML = def.name;
    $.replaceViewport(page);
    console.log(def);
  };

  return {
    name: "logviewer_logfile_editor",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

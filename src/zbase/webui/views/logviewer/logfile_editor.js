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
    $("textarea.regex", page).value = def.regex;

    renderSourceFields($(".editor_pane.source_fields", page), def.source_fields);
    renderRowFields($(".editor_pane.row_fields", page), def.row_fields);

    $.replaceViewport(page);
    console.log(def);
  };

  var renderSourceFields = function(elem, fields) {
    console.log(fields);
    var tbody = $("tbody", elem);
    var tr_tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_row_tpl");

    fields.forEach(function(field) {
      var tr = tr_tpl.cloneNode(true);
      $(".name", tr).innerHTML = field.name;
      $(".type", tr).innerHTML = field.type;
      $(".format", tr).innerHTML = field.format;

      tbody.appendChild(tr);
    });
  };

  var renderRowFields = function(elem, fields) {
    console.log(fields);
    var tbody = $("tbody", elem);
    var tr_tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_row_tpl");

    fields.forEach(function(field) {
      var tr = tr_tpl.cloneNode(true);
      $(".name", tr).innerHTML = field.name;
      $(".type", tr).innerHTML = field.type;
      $(".format", tr).innerHTML = field.format;

      tbody.appendChild(tr);
    });

  };

  return {
    name: "logviewer_logfile_editor",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

ZBase.registerView((function() {

  var load = function() {
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
    var page = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_list_tpl");

    var tr_tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_list_row_tpl");

    var tbody = $("tbody", page);
    logfiles.forEach(function(def) {
      var tr = tr_tpl.cloneNode(true);
      $(".name", tr).innerHTML = def.name;

      $("z-dropdown", tr).addEventListener("change", function() {
        switch (this.getValue()) {
          case "open_viewer":
            $.navigateTo("/a/logs/view/" + def.name);
            return;

          case "edit":
            console.log("edit schema");
            return;
        }
      });

      tbody.appendChild(tr);
    });

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  return {
    name: "logviewer_logfile_list",
    loadView: function(params) { load(); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

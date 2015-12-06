ZBase.registerView((function() {

  var load = function(path) {
    $.showLoader();
    ZBaseMainMenu.show();
    HeaderWidget.hideBreadCrumbs();

    $.httpGet("/api/v1/logfiles", function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response).logfile_definitions, path);
      } else {
        $.fatalError();
      }

      $.hideLoader();
    });
  }

  var render = function(logfiles, path) {
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

      var links = tr.querySelectorAll("a");
      for (var i = 0; i < links.length; i++) {
        //logviewer path
        links[i].href = "/a/logs/view/" + def.name;
      }

      $("z-dropdown", tr).addEventListener("change", function() {
        switch (this.getValue()) {
          case "open_viewer":
            $.navigateTo("/a/logs/view/" + def.name);
            return;

          case "edit":
            $.navigateTo("/a/logs/" + def.name);
            return;
        }
      });

      tbody.appendChild(tr);
    });


    $.onClick($(".link.add", page), displayAddLogfileModal);
    var add_modal = $("z-modal.add_logfile", page);
    $.onClick($("button.submit", add_modal), addLogfile);
    $.onClick($("button.close", add_modal), function() {
      add_modal.close();
    });

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var displayAddLogfileModal = function() {
    var modal = $(".zbase_logviewer .logfile_list z-modal.add_logfile");
    var tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_list_modal_tpl");

    $.replaceContent($(".z-modal-content", modal), tpl);
    modal.show();
    $("input", modal).focus();
  };

  var addLogfile = function() {
    var input = $(".zbase_logviewer .logfile_list z-modal.add_logfile input");

    if (input.value == 0) {
      $(".zbase_logviewer .logfile_list z-modal.add_logfile .error_note")
          .classList.remove("hidden");
      input.classList.add("error");
      input.focus();
      return;
    }

    var url = "/api/v1/logfiles/add_logfile?logfile=" + $.escapeHTML(input.value);
    $.httpPost(url, "", function(r) {
      if (r.status == 201) {
        var def = JSON.parse(r.response);
        $.navigateTo("/a/logs/view/" + def.name);
      } else {
        $.fatalError();
      }
    });
  };

  return {
    name: "logviewer_logfile_list",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

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
      console.log(def);
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
  };

  var addLogfile = function() {
    var input = $(".zbase_logviewer .logfile_list z-modal.add_logfile input");

    if (input.value == 0) {
      $(".zbase_logviewer .logfile_list z-modal.add_logfile .error_note")
          .classList.remove("hidden");
      input.classList.add("error");
      return;
    }

    var url = "/api/v1/logfiles/add_logfile?name=" + $.escapeHTML(input.value);
    alert("POST " + url);
    //TODO redirect to edit schema page
  };

  return {
    name: "logviewer_logfile_list",
    loadView: function(params) { load(); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

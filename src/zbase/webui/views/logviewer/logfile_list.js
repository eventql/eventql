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

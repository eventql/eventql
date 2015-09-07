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

  var createNewLogfile = function() {
    alert("not yet implemented");
  }

  var render = function(logfiles) {
    var page = $.getTemplate(
        "views/datastore_logfiles",
        "zbase_datastore_logfiles_list_tpl");

    var menu = DatastoreMenu();
    menu.render($(".zbase_datastore_menu_sidebar", page));

    var tbody = $("tbody", page);
    logfiles.forEach(function(logfile) {
      renderRow(tbody, logfile);
    });

    $.onClick($(".add_logfile_definition", page), createNewLogfile);

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderRow = function(tbody, logfile) {
    var elem = $.getTemplate(
        "views/datastore_logfiles",
        "zbase_datastore_logfiles_list_row_tpl");

    var url = "/a/logviewer/" + logfile.name;

    var source_field_names = logfile.source_fields.map(function(f) {
      return f.name;
    });

    var row_field_names = logfile.row_fields.map(function(f) {
      return f.name;
    });

    $(".logfile_name a", elem).innerHTML = logfile.name;
    $(".logfile_name a", elem).href = url;
    $(".edit_link", elem).href = url;
    $(".logfile_format", elem).innerHTML = $.wrapText(logfile.regex);
    $(".logfile_source_fields", elem).innerHTML = source_field_names.join(", ");
    $(".logfile_row_fields", elem).innerHTML = row_field_names.join(", ");

    console.log(logfile);
    //var tr = document.createElement("tr");
    //tr.innerHTML = 
    //    "<td><a href='" + url + "'>" + logfile.name + "</a></td>" +
    //    "<td><a href='" + url + "'>&mdash;</a></td>" +
    //    "<td><a href='" + url + "'>&mdash;</a></td>";

    //$.onClick(elem, function() { $.navigateTo(url); });
    tbody.appendChild(elem);
  }

  return {
    name: "datastore_logfiles",
    loadView: function(params) { load(params.url); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

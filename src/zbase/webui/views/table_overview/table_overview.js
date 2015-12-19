ZBase.registerView((function() {
  var kPathPrefix = "/a/datastore/tables/";

  var load = function(path) {
    var table_id = path.substr(kPathPrefix.length);

    $.showLoader();

    $.httpGet("/api/v1/tables/" + table_id, function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response).table);
      } else {
        renderError(r.statusText, kPathPrefix + table_id);
      }
      $.hideLoader();
    });
  };

  var render = function(schema) {
    console.log(schema);
    var page = $.getTemplate(
        "views/table_overview",
        "zbase_table_overview_main_tpl");

    var table_breadcrumb = $(".table_name_breadcrumb", page);
    table_breadcrumb.innerHTML = schema.name;
    table_breadcrumb.href = kPathPrefix + schema.name;

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderError = function(msg, path) {
    var error_elem = document.createElement("div");
    error_elem.classList.add("zbase_error");
    error_elem.innerHTML = 
        "<span>" +
        "<h2>We're sorry</h2>" +
        "<h1>An error occured.</h1><p>" + msg + "</p> " +
        "<p>Please try it again or contact support if the problem persists.</p>" +
        "<a href='/a/datastore/tables' class='z-button secondary'>" +
        "<i class='fa fa-arrow-left'></i>&nbsp;Back</a>" +
        "<a href='" + path + "' class='z-button secondary'>" + 
        "<i class='fa fa-refresh'></i>&nbsp;Reload</a>" +
        "</span>";

    $.replaceViewport(error_elem);
  };




  return {
    name: "table_overview",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

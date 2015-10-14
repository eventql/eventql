ZBase.registerView((function() {

  var load = function(path) {
    var path_prefix = "/a/seller/";
    var seller_id = path.substr(path_prefix.length);


    var page = $.getTemplate(
        "views/seller",
        "per_seller_stats_main_tpl");


    $.handleLinks(page);
    $.replaceViewport(page);
  };

  return {
    name: "per_seller_stats",
    loadView: function(params) {load(params.path)},
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

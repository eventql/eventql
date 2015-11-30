ZBase.registerView((function() {
  var path_prefix = "/a/apps/dawanda_seller_analytics/traffic/";
  var shop_id;

  var load = function(path) {
    query_mgr = EventSourceHandler();
    shop_id = UrlUtil.getPath(path).substr(path_prefix.length);


    perSellerStats.renderLayout(query_mgr, path_prefix, shop_id);
  };

  var destroy = function() {

  };

  return {
    name: "per_seller_traffic",
    loadView: function(params) {load(params.path)},
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

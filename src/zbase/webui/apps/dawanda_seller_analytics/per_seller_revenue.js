ZBase.registerView((function() {
  var path_prefix = "/a/apps/dawanda_seller_analytics/revenue/";
  var shop_id;
  var query_mgr;

  var load = function(path) {

  };

  var destroy = function() {
    if (query_mgr) {
      query_mgr.closeAll();
    }
  };

  return {
    name: "per_seller_revenue",
    loadView: function(params) {load(params.path)},
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

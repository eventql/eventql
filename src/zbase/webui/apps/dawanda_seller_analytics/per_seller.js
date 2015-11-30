var perSellerStats = (function() {
  var renderLayout = function(query_mgr, path_prefix, shop_id) {
    var page = $.getTemplate(
        "views/seller",
        "per_seller_layout_main_tpl");

    $("zbase-breadcrumbs-section a.shop_id", page).href = path_prefix + shop_id;
    $("h2.pagetitle .shop_id", page).innerHTML = shop_id;

    var tabs = page.querySelectorAll("z-tab");
    for (var i = 0; i < tabs.length; i++) {
      var tab_path = tabs[i].getAttribute("data-path");
      $("a", tabs[i]).href = tab_path + shop_id;
      if (tab_path == path_prefix) {
        tabs[i].setAttribute("data-active", "active");
      }
    }

    $.handleLinks(page);
    $.replaceViewport(page);

    loadShopName(shop_id);
  };

  var loadShopName = function(shop_id) {
    var query_str = "SELECT `title` FROM db.shops WHERE user_id = " +
      $.escapeHTML(shop_id);

    var query = query_mgr.get(
      "shop_name",
      "/api/v1/sql?format=json_sse&query=" + encodeURIComponent(query_str));

    query.addEventListener("result", function(e) {
      query_mgr.close("shop_name");
      var result = JSON.parse(e.data).results[0];
      $(".zbase_seller_stats h2.pagetitle .shop_name").innerHTML = result.rows[0];
      $(".zbase_seller_stats zbase-breadcrumbs-section a.shop_id").innerHTML = result.rows[0];
    });

    query.addEventListener("error", function(e) {
      query_mgr.close("shop_name");
      $(".zbase_seller_stats zbase-breadcrumbs-section a.shop_id", page).innerHTML = shop_id;
    });
  };


  return {
    renderLayout: renderLayout
  }
})();

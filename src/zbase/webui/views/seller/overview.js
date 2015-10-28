ZBase.registerView((function() {
  var load = function(path) {
    var page = $.getTemplate(
        "views/seller",
        "seller_overview_main_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), path);

    $.onClick($("z-checkbox.premium", page), paramChanged);
    $("z-dropdown.metrics", page).addEventListener("change", paramChanged);
    $("z-search.seller", page).addEventListener("z-search-submit", paramChanged);


    $.handleLinks(page);
    $.replaceViewport(page);

    setParamSeller(UrlUtil.getParamValue(path, "seller"));
    setParamPremiumSeller(UrlUtil.getParamValue(path, "premium"));
    setParamMetrics(UrlUtil.getParamValue(path, "metrics"));

    renderTable($(".zbase_seller_stats .zbase_seller_overview"), []);
  };

  var renderTable = function(elem, data) {
    var metrics = $(".zbase_seller_stats z-dropdown.metrics").getValue().split(",");

    var table_tpl = $.getTemplate(
        "views/seller_overview",
        "seller_overview_table_tpl");

    metrics.forEach(function(metric) {
      $("th." + metric, table_tpl).classList.remove("hidden");
    });

    var tr_tpl = $.getTemplate(
        "views/seller_overview",
        "seller_overview_row_tpl");

    var tbody = $("tbody", table_tpl);
    for (var i = 0; i < 10; i++) {
      tbody.appendChild(tr_tpl.cloneNode(true));
    }

    $.replaceContent(elem, table_tpl);

  };

  var setParamSeller = function(value) {
    $(".zbase_seller_stats z-search.seller input").value = value;
  };

  var setParamPremiumSeller = function(value) {
    if (value) {
      $(".zbase_seller_stats z-checkbox.premium").setAttribute(
        "data-active", "active");
    } else {
      $(".zbase_seller_stats z-checkbox.premium").removeAttribute("data-active");
    }
  };

  var setParamMetrics = function(metrics) {
    if (metrics) {
      metrics = metrics.split(",");
    } else {
      //default selection
      metrics = [
        "gmv_eurcent", "num_purchases", "listview_ctr_search_page",
        "listview_ctr_catalog_page", "listview_ctr_shop_page"];
    }

    $(".zbase_seller_stats z-dropdown.metrics").setValue(metrics);
  };

  var getQueryString = function() {
    return {
      seller: $(".zbase_seller_stats z-search.seller").getValue(),
      metrics: $(".zbase_seller_stats z-dropdown.metrics").getValue(),
      premium: $(".zbase_seller_stats z-checkbox.premium").hasAttribute(
          "data-active")
    }
  };

  var paramChanged = function() {
    $.navigateTo("/a/seller?" + $.buildQueryString(getQueryString()));
  };

  return {
    name: "seller_overview",
    loadView: function(params) {load(params.path);},
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

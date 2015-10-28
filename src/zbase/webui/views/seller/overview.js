ZBase.registerView((function() {
  var load = function(path) {
    var page = $.getTemplate(
        "views/seller",
        "seller_overview_main_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), path);


    var tbody = $("tbody", page);
    var tr_tpl = $.getTemplate(
        "views/seller_overview",
        "seller_overview_row_tpl");

    for (var i = 0; i < 10; i++) {
      tbody.appendChild(tr_tpl.cloneNode(true));
    }


    $.handleLinks(page);
    $.replaceViewport(page);

    setParamSeller(UrlUtil.getParamValue(path, "seller"));
    setParamPremiumSeller(UrlUtil.getParamValue(path, "premium"));

    $.onClick($(".zbase_seller_stats z-checkbox.premium"), paramChanged);
    $(".zbase_seller_stats z-search.seller").addEventListener(
        "z-search-submit",
        paramChanged,
        false);
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

  var getQueryString = function() {
    return {
      seller: $(".zbase_seller_stats z-search.seller").getValue(),
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

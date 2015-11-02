ZBase.registerView((function() {
  var query_mgr;

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

    query_mgr = EventSourceHandler();
    var query_str =
      "select shop_id, sum(gmv_eurcent) gmv_eurcent, " +
      "sum(gmv_per_transaction_eurcent) / count(1) gmv_per_transaction_eurcent, " +
      "sum(num_purchases) num_purchases, " +
      "sum(shop_page_views) shop_page_views, " +
      "sum(product_page_views) product_page_views, " +
      "sum(listview_views_search_page) + sum(listview_views_shop_page) + sum(listview_views_catalog_page) + sum(listview_views_ads) + sum(listview_views_recos) as total_listviews " +
      "from shop_stats.last30d " +
      "where gmv_eurcent > 0 group by shop_id order by gmv_eurcent desc limit 50;";

    var query = query_mgr.get(
      "sql_query",
      "/api/v1/sql_stream?query=" + encodeURIComponent(query_str));

    query.addEventListener("result", function(e) {
      query_mgr.close("sql_query");
      var data = JSON.parse(e.data).results[0];
      hideLoader();
      renderTable($(".zbase_seller_overview table"), data);
    });

    query.addEventListener("error", function(e) {
      query_mgr.close("sql_query");
      $.fatalError();
    }, false);

    query.addEventListener("status", function(e) {
      renderQueryProgress(JSON.parse(e.data));
    }, false);
  };

  var destroy = function() {
    query_mgr.closeAll();
  };

  var renderQueryProgress = function(progress) {
    QueryProgressWidget.render(
        $(".zbase_seller_stats .query_progress"),
        progress);
  };

  var hideLoader = function() {
    $(".zbase_seller_stats .zbase_seller_overview").classList.remove("hidden");
    $(".zbase_seller_stats .query_progress").classList.add("hidden");
  };

  var renderTable = function(elem, result) {
    var metrics = $(".zbase_seller_stats z-dropdown.metrics").getValue().split(",");

    var table_tpl = $.getTemplate(
        "views/seller_overview",
        "seller_overview_table_tpl");

    result.columns.forEach(function(metric) {
      $("th." + metric, table_tpl).classList.remove("hidden");
    });

    var tr_tpl = $.getTemplate(
        "views/seller_overview",
        "seller_overview_row_tpl");

    var tbody = $("tbody", table_tpl);
    result.rows.forEach(function(row) {
      var tr = tr_tpl.cloneNode(true);
      var url = "/a/apps/dawanda_seller_analytics/" + row[0]

      for (var i = 0; i < row.length; i++) {
        var td = $("td." + result.columns[i], tr);
        var a = $("a", td);
        a.innerHTML = row[i];
        a.href = url;
        td.classList.remove("hidden");
      }

      tbody.appendChild(tr);
    });

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
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

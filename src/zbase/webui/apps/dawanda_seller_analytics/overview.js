ZBase.registerView((function() {
  var query_mgr;
  var path_prefix = "/a/apps/dawanda_seller_analytics";

  var load = function(path) {
    var result;
    var page = $.getTemplate(
        "views/seller",
        "seller_overview_main_tpl");

    var main_menu = ZBaseMainMenu();
    main_menu.render($(".zbase_main_menu", page), path);

    $.onClick($("z-checkbox.premium", page), paramChanged);
    $("z-dropdown.metrics", page).addEventListener("change", function() {
      renderTable($(".zbase_seller_overview table"), result, path);
      $.pushHistoryState(path_prefix + "?" + $.buildQueryString(getQueryString()));
    });
    $("z-search.seller", page).addEventListener("z-search-submit", function(e) {
      $.navigateTo(path_prefix + "/" + e.detail.value);
    });

    $.handleLinks(page);
    $.replaceViewport(page);

    setParamPremiumSeller(UrlUtil.getParamValue(path, "premium"));
    setParamMetrics(UrlUtil.getParamValue(path, "metrics"));
    setParamsOrder(
        UrlUtil.getParamValue(path, "order_by"),
        UrlUtil.getParamValue(path, "order_fn"));

    var order = getParamsOrder();

    var query_str =
      "select shop_id, " +
      "sum(num_active_products) num_active_products, " +
      "sum(num_listed_products) num_listed_products, " +
      "sum(num_purchases) num_purchases, " +
      "sum(num_refunds) num_refunds," +
      "sum(refund_rate) refund_rate," +
      "sum(gmv_eurcent) gmv_eurcent, " +
      "sum(refunded_gmv_eurcent) refunded_gmv_eurcent," +
      "sum(gmv_per_transaction_eurcent) / count(1) gmv_per_transaction_eurcent, " +
      "sum(shop_page_views) shop_page_views, " +
      "sum(product_page_views) product_page_views, " +
      "sum(listview_views_search_page) + sum(listview_views_shop_page) + sum(listview_views_catalog_page) + sum(listview_views_ads) + sum(listview_views_recos) as total_listviews, " +
      "sum(listview_views_search_page) listview_views_search_page," +
      "sum(listview_clicks_search_page) listview_clicks_search_page," +
      "sum(listview_ctr_search_page) / count(1) listview_ctr_search_page," +
      "sum(listview_views_catalog_page) listview_views_catalog_page," +
      "sum(listview_clicks_catalog_page) listview_clicks_catalog_page," +
      "sum(listview_ctr_catalog_page) / count(1) listview_ctr_catalog_page," +
      "sum(listview_views_shop_page) listview_views_shop_page," +
      "sum(listview_clicks_shop_page) listview_clicks_shop_page," +
      "sum(listview_ctr_shop_page) / count(1) listview_ctr_shop_page," +
      "sum(listview_views_ads) listview_views_ads, " +
      "sum(listview_clicks_ads) listview_clicks_ads, " +
      "sum(listview_ctr_ads) / count(1) listview_ctr_ads, " +
      "sum(listview_views_recos) listview_views_recos, " +
      "sum(listview_clicks_recos) listview_clicks_recos, " +
      "sum(listview_ctr_recos) / count(1) listview_ctr_recos " +
      "from shop_stats.last30d " +
      "where gmv_eurcent > 0 group by shop_id order by " + order.order_by +
      " " + order.order_fn + " limit 20;";

    query_mgr = EventSourceHandler();
    var query = query_mgr.get(
      "sql_query",
      "/api/v1/sql_stream?query=" + encodeURIComponent(query_str));

    query.addEventListener("result", function(e) {
      query_mgr.close("sql_query");
      result = JSON.parse(e.data).results[0];
      hideLoader();
      renderTable($(".zbase_seller_overview table"), result, path);
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

  var renderTable = function(elem, result, path) {
    var metrics = {shop_id: true};
    $(".zbase_seller_stats z-dropdown.metrics")
        .getValue()
        .split(",")
        .forEach(function(metric) {
          metrics[metric] = true;
        });

    var table_tpl = $.getTemplate(
        "views/seller_overview",
        "seller_overview_table_tpl");

    result.columns.forEach(function(metric) {
      if (metrics[metric]) {
        var th = $("th." + metric, table_tpl);
        th.classList.remove("hidden");
        $.onClick($(".asc", th), function() {
          orderByParamChanged(metric, "asc");
        });
        $.onClick($(".desc", th), function() {
          orderByParamChanged(metric, "desc");
        });
      }
    });

    var tr_tpl = $.getTemplate(
        "views/seller_overview",
        "seller_overview_row_tpl");

    var tbody = $("tbody", table_tpl);
    result.rows.forEach(function(row) {
      var tr = tr_tpl.cloneNode(true);
      var url = "/a/apps/dawanda_seller_analytics/" + row[0]

      for (var i = 0; i < row.length; i++) {
        if (metrics[result.columns[i]]) {
          var td = $("td." + result.columns[i], tr);
          var a = $("a", td);
          a.innerHTML = ZBaseSellerMetrics[result.columns[i]].print(row[i]);
          a.href = url;
          td.classList.remove("hidden");
        }
      }

      tbody.appendChild(tr);
    });


    $.handleLinks(table_tpl);
    $.replaceContent(elem, table_tpl);

    setParamsOrder(
        UrlUtil.getParamValue(path, "order_by"),
        UrlUtil.getParamValue(path, "order_fn"));
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
      $(".zbase_seller_stats z-dropdown.metrics").setValue(metrics.split(","));
    }
  };

  var setParamsOrder = function(order_by, order_fn) {
    if (order_by && order_fn) {
      if ($(".zbase_seller_stats thead")) {
        $(".zbase_seller_stats th.order_by").classList.remove("order_by");
        $(".zbase_seller_stats th." + order_by).classList.add("order_by");
      }

      var table = $(".zbase_seller_stats table");
      table.setAttribute("data-order-by", order_by);
      table.setAttribute("data-order-fn", order_fn);
    }
  };

  var getParamsOrder = function() {
    var table = $(".zbase_seller_stats table");

    return {
      order_by: table.getAttribute("data-order-by"),
      order_fn: table.getAttribute("data-order-fn")
    };
  }

  var getQueryString = function() {
    var params = getParamsOrder();
    params.metrics = $(".zbase_seller_stats z-dropdown.metrics").getValue();
    params.premium = $(".zbase_seller_stats z-checkbox.premium").hasAttribute(
          "data-active");

    return params;
  };

  var paramChanged = function() {
    $.navigateTo(path_prefix + "?" + $.buildQueryString(getQueryString()));
  };

  var orderByParamChanged = function(order_by, order_fn) {
    setParamsOrder(order_by, order_fn);
    paramChanged();
  };

  return {
    name: "seller_overview",
    loadView: function(params) {load(params.path);},
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

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
      renderTable($(".zbase_seller_overview table.overview"), result, path);
      $.pushHistoryState(path_prefix + "?" + $.buildQueryString(getQueryParams()));
    });
    $("z-search.seller", page).addEventListener("z-search-submit", function(e) {
      $.navigateTo(path_prefix + "/" + e.detail.value);
    });

    $.handleLinks(page);
    $.replaceViewport(page);

    setParamPremiumSeller(UrlUtil.getParamValue(path, "premium"));
    setParamMetrics(UrlUtil.getParamValue(path, "metrics"));
    setParamOffset(UrlUtil.getParamValue(path, "offset"));
    setParamsOrder(
        UrlUtil.getParamValue(path, "order_by"),
        UrlUtil.getParamValue(path, "order_fn"));

    query_mgr = EventSourceHandler();
    var query = query_mgr.get(
      "sql_query",
      "/api/v1/sql?format=json_sse&query=" + encodeURIComponent(getSQLQuery()));

    query.addEventListener("result", function(e) {
      query_mgr.close("sql_query");
      result = JSON.parse(e.data).results[0];
      hideLoader();
      renderTable($(".zbase_seller_overview table.overview"), result, path);
      setPagination(result);

      var until = Date.now();
      $(".zbase_seller_overview .time_range").innerHTML =
         DateUtil.printDate(until - DateUtil.millisPerDay) + "-" +
         DateUtil.printDate(until);
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

  var getSQLQuery = function() {
    var order = getParamsOrder();
    var offset = getParamOffset();

    var query_str =
      "select count(1) as days, shop_id, " +
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
      "from shop_stats.last30d where ";

    query_str +=
      order.order_by + " > 0 group by shop_id order by " + order.order_by +
      " " + order.order_fn + " limit 20 offset " + offset + ";";

    console.log(query_str);
    return query_str;
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
      var url = "/a/apps/dawanda_seller_analytics/" + row[1]

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

  var setPagination = function(result) {
    var params = getQueryParams();
    params.offset = parseInt(params.offset, 10);
    params.limit = parseInt(params.limit, 10);

    //backward
    if (params.offset > 0) {
      params.offset = params.offset - params.limit;
    }
    $(".zbase_seller_stats .pager .back").href =
        path_prefix + "?" + $.buildQueryString(params);


    //forward
    params.offset = params.offset + params.limit;
    $(".zbase_seller_stats .pager .for").href =
        path_prefix + "?" + $.buildQueryString(params);
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

      var table = $(".zbase_seller_stats table.overview");
      table.setAttribute("data-order-by", order_by);
      table.setAttribute("data-order-fn", order_fn);
    }
  };

  var setParamOffset = function(offset) {
    if (!offset) {
      offset = 0;
    }

    $(".zbase_seller_stats .pager").setAttribute("data-offset", offset);
  };

  var getParamsOrder = function() {
    var table = $(".zbase_seller_stats table.overview");

    return {
      order_by: table.getAttribute("data-order-by"),
      order_fn: table.getAttribute("data-order-fn")
    };
  };

  var getParamOffset = function() {
    return parseInt(
        $(".zbase_seller_stats .pager").getAttribute("data-offset"), 10);
  };

  var getQueryParams = function() {
    var params = getParamsOrder();
    params.offset = getParamOffset();
    params.metrics = $(".zbase_seller_stats z-dropdown.metrics").getValue();
    params.premium = $(".zbase_seller_stats z-checkbox.premium").hasAttribute(
          "data-active");
    params.limit = "20";

    return params;
  };

  var paramChanged = function() {
    $.navigateTo(path_prefix + "?" + $.buildQueryString(getQueryParams()));
  };

  var orderByParamChanged = function(order_by, order_fn) {
    setParamsOrder(order_by, order_fn);
    setParamOffset(0);
    paramChanged();
  };

  return {
    name: "seller_overview",
    loadView: function(params) {load(params.path);},
    unloadView: destroy,
    handleNavigationChange: load
  };

})());

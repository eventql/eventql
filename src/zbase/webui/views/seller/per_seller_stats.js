ZBase.registerView((function() {
  var path_prefix = "/a/seller/";
  var seller_id;

  var load = function(path) {
    seller_id = path.substr(path_prefix.length);

    var page = $.getTemplate(
        "views/seller",
        "per_seller_stats_main_tpl");

    $("z-daterangepicker", page).addEventListener("select", paramChanged, false);

    $.handleLinks(page);
    $.replaceViewport(page);

    setParamsFromAndUntil(
        UrlUtil.getParamValue(path, "from"),
        UrlUtil.getParamValue(path, "until"));

    //REMOVE ME
    var data = {
      aggregates: {
        product_page_impressions: 64,
        product_listview_impressions: 58,
        product_listview_clicks: 37,
        product_listview_ctr: 0.34
      },
      timeseries: [
        {
          product_page_impressions: 3,
          product_listview_impressions: 10,
          product_listview_clicks: 9,
          product_listview_ctr: 0.34
        },
        {
          product_page_impressions: 12,
          product_listview_impressions: 8,
          product_listview_clicks: 3,
          product_listview_ctr: 0.34
        },
        {
          product_page_impressions: 10,
          product_listview_impressions: 7,
          product_listview_clicks: 10,
          product_listview_ctr: 0.34
        },
        {
          product_page_impressions: 17,
          product_listview_impressions: 14,
          product_listview_clicks: 11,
          product_listview_ctr: 0.34
        },
        {
          product_page_impressions: 18,
          product_listview_impressions: 16,
          product_listview_clicks: 12,
          product_listview_ctr: 0.34
        }
      ]
    };
    //REMOVEME END

    render(data);
  };

  var render = function(data) {
    var sparkline_values = {};
    data.timeseries.forEach(function(serie) {
      for (metric in serie) {
        if (!sparkline_values[metric]) {
          sparkline_values[metric] = [];
        }
        sparkline_values[metric].push(serie[metric]);
      }
    });

    for (var metric in data.aggregates) {
      var pane = $(".zbase_seller_stats ." + metric);
      $(".num", pane).innerHTML = data.aggregates[metric];
      $("z-sparkline", pane).setAttribute(
          "data-sparkline",
          sparkline_values[metric].join(","));

      $(".zbase_seller_stats z-tooltip." + metric).init($("i.help", pane));
    }
  };

  var setParamsFromAndUntil = function(from, until) {
    if (!until) {
      until = DateUtil.getStartOfDay(Date.now());
    }

    if (!from) {
      from = until - DateUtil.millisPerDay * 30;
    }

    $(".zbase_seller_stats z-daterangepicker").setValue(
        from, until);
  };

  var getUrl = function() {
    var params = $(".zbase_seller_stats z-daterangepicker").getValue();
    return path_prefix + seller_id + "?" + $.buildQueryString(params);
  };

  var paramChanged = function(e) {
    $.navigateTo(getUrl());
  };


  return {
    name: "per_seller_stats",
    loadView: function(params) {load(params.path)},
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

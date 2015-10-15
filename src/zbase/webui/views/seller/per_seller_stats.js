ZBase.registerView((function() {

  var load = function(path) {

    var path_prefix = "/a/seller/";
    var seller_id = path.substr(path_prefix.length);

    var page = $.getTemplate(
        "views/seller",
        "per_seller_stats_main_tpl");

    //REMOVE ME
    var data = {
      aggregates: {
        product_page_impressions: 64,
        product_listview_impressions: 58,
        product_listview_clicks: 37
      },
      timeseries: [
        {
          product_page_impressions: 3,
          product_listview_impressions: 10,
          product_listview_clicks: 9
        },
        {
          product_page_impressions: 12,
          product_listview_impressions: 8,
          product_listview_clicks: 3
        },
        {
          product_page_impressions: 10,
          product_listview_impressions: 7,
          product_listview_clicks: 10
        },
        {
          product_page_impressions: 17,
          product_listview_impressions: 14,
          product_listview_clicks: 11
        },
        {
          product_page_impressions: 18,
          product_listview_impressions: 16,
          product_listview_clicks: 12
        }
      ]
    };
    //REMOVEME END

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
      $("." + metric + " .num", page).innerHTML = data.aggregates[metric];
      $("." + metric + " z-sparkline", page).setAttribute(
          "data-sparkline",
          sparkline_values[metric].join(","));
    }

    $.handleLinks(page);
    $.replaceViewport(page);
    $.hideLoader();
  };



  return {
    name: "per_seller_stats",
    loadView: function(params) {load(params.path)},
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

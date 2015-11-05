var SellerCSVDownload = (function() {
  var render = function(elem) {
    var tpl = $.getTemplate(
        "apps/dawanda_seller_analytics",
        "seller_csv_download_main_tpl");

    //last 30 days
    var until = until = DateUtil.getStartOfDay(Date.now());
    var from = until - DateUtil.millisPerDay * 30;
    $("z-daterangepicker", tpl).setValue(from, until);

    $.onClick($("button.download", tpl), startDownload);

    $.replaceContent(elem, tpl);
  };

  var startDownload = function() {
    var params = $(".seller_csv_download z-daterangepicker").getValue();
    params.columns = $(".seller_csv_download z-dropdown.columns").getValue();
    params.order_by = $(".seller_csv_download z-dropdown.order_by").getValue();
    params.order_fn = $(".seller_csv_download z-dropdown.order_fn").getValue();
    console.log("Download CSV: ", params);
  };

  return {
    render: render
  }
})();

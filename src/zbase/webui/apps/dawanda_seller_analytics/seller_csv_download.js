var SellerCSVDownload = (function() {
  var render = function(elem) {
    var tpl = $.getTemplate(
        "apps/dawanda_seller_analytics",
        "seller_csv_download_main_tpl");

    $.replaceContent(elem, tpl);
    console.log("render csv download");
  }

  return {
    render: render
  }
})();

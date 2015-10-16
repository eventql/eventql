ZBase.registerView((function() {
  var load = function(path) {
    var page = $.getTemplate(
        "views/seller",
        "seller_overview_main_tpl");

    //REMOVEME
    var seller = [
      {id: 13008, name: "meko", is_premium: true},
      {id: 13008, name: "meko", is_premium: false},
      {id: 13008, name: "meko", is_premium: true}
    ];
    //REMOVEME END

    var tbody = $("tbody", page);
    seller.forEach(function(s) {
      var tr = document.createElement("tr");
      var url = "/a/seller/" + s.id;
      var html =
        "<td><a href='" + url + "'>" + s.name +
        "</a></td><td><a href='" + url + "'>" + s.id;
      if (s.is_premium) {
        html += "</a></td><td class='icon'><a href='" + url +
          "'><i class='fa fa-star'></i></a>";
      } else {
        html += "</a></td><td><a href='" + url + "'></a></td>";
      }
      tr.innerHTML = html;
      tbody.appendChild(tr);
    });

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

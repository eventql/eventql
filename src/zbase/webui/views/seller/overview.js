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
      var html = "<td>" + s.name + "</td><td>" + s.id;
      if (s.is_premium) {
        html += "</td><td class='icon'><i class='fa fa-star'></i>";
      } else {
        html += "</td><td></td>";
      }
      tr.innerHTML = html;
      tbody.appendChild(tr);

      $.onClick(tr, function() {
        $.navigateTo("/a/seller/" + s.id);
      });
    });

    $.handleLinks(page);
    $.replaceViewport(page);

    setParamSeller(UrlUtil.getParamValue(path, "seller"));
    setParamCategory(UrlUtil.getParamValue(path, "category"));
    setParamPremiumSeller(UrlUtil.getParamValue(path, "premium"));

    $.onClick($(".zbase_seller_stats z-checkbox.premium"), paramChanged);

    var category_search = $(".zbase_seller_stats z-search.category");
    category_search.addEventListener(
        "z-search-submit",
        paramChanged,
        false);
    category_search.addEventListener(
        "z-search-autocomplete",
        categoryAutocomplete,
        false);

    var seller_search = $(".zbase_seller_stats z-search.seller");
    seller_search.addEventListener(
        "z-search-submit",
        paramChanged,
        false);
  };

  var setParamSeller = function(value) {
    $(".zbase_seller_stats z-search.seller input").value = value;
  };

  var setParamCategory = function(value) {
    $(".zbase_seller_stats z-search.category input").value = value;
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
      category: $(".zbase_seller_stats z-search.category").getValue(),
      premium: $(".zbase_seller_stats z-checkbox.premium").hasAttribute(
          "data-active")
    }
  };

  var paramChanged = function() {
    $.navigateTo("/a/seller?" + $.buildQueryString(getQueryString()));
  };

  var categoryAutocomplete = function(e) {
    var term = e.detail.value;

    //REMOVEME
    var suggestions = [
      {
        data_value: 1,
        query: "Mode (1)"
      },
      {
        data_value: 1,
        query: "Mode (1)"
      },
      {
        data_value: 1,
        query: "Mode (1)"
      }
    ];
    //REMOVEME END

    this.autocomplete(term, suggestions);
  };

  return {
    name: "seller_overview",
    loadView: function(params) {load(params.path);},
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

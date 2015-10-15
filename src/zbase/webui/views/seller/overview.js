ZBase.registerView((function() {
  var load = function() {
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
  };

  return {
    name: "seller_overview",
    loadView: function(params) {load();},
    unloadView: function() {},
    handleNavigationChange: function() {load();}
  };

})());

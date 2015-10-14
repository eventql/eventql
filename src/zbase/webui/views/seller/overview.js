ZBase.registerView((function() {
  var load = function() {
    var page = $.getTemplate(
        "views/seller",
        "seller_overview_main_tpl");

    //REMOVEME
    var seller = [
      {id: 13008, name: "meko"},
      {id: 13008, name: "meko"},
      {id: 13008, name: "meko"}
    ];
    //REMOVEME END

    var tbody = $("tbody", page);
    seller.forEach(function(s) {
      var tr = document.createElement("tr");
      tr.innerHTML = "<td>" + s.name + "</td><td>" + s.id + "</td>";
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
    loadView: load,
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

ZBase.registerView((function() {
    var load = function() {
      console.log("load report view");
    };

    return {
    name: "report",
    loadView: function(params) { load(); },
    unloadView: function() {},
    handleNavigationChange: load
  };

})());

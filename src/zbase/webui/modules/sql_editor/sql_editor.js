ZBase.registerView((function() {

  return {
    name: "sql_editor",

    loadView: function(params) {
      console.log("load view sql editor", params);
    },
    unloadView: function() {
      console.log("unload view sql editor");
    },
    handleNavigationChange: function(url){
      console.log("handle nav change sql editor");
    }
  };

})());

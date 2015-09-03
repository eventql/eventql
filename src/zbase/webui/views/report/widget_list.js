var WidgetList = function() {
  var widgets = [];

  var init = function(elem) {
    console.log("init");
    
  };

  var getJSON = function() {
    return {widgets: widgets};
  };

  var setJSON = function(new_json) {
    json = new_json;
    console.log(json);
  };

  var setEditable = function(is_editable) {

  };


  return {
    init: init,
    getJSON: getJSON,
    setJSON: setJSON,
    setEditable: setEditable
  }
};

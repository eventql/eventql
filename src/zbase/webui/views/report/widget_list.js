var WidgetList = function() {
  var widgets = [];

  var init = function(elem) {
    console.log("init");
    
  };

  var getJSON = function() {
    return widgets;
  };

  var setJSON = function(new_widgets) {
    widgets = new_widgets;
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

var WidgetList = function(elem, widget_definitions) {
  var widgets = [];

  var getJSON = function() {
    //foreach widget getJson
    //return widgets;
  };

  //var setJSON = function(new_widgets) {
  //  widgets = new_widgets;
  //  var tpl = $.getTemplate(
  //        "views/report",
  //        "zbase_report_widget_main_tpl");

  //  pane.innerHTML = "";
  //  widgets.forEach(function(name) {
  //    var elem = tpl.cloneNode(true);
  //    var widget = getWidget(name);
  //    widget.renderContent($(".zbase_report_widget_pane", elem));


  //    $.onClick($(".zbase_report_widget_header .edit", elem), function() {
  //      renderEditView(widget);
  //    });

  //    pane.appendChild(elem);
  //  });
  //};

  var setEditable = function(is_editable) {
    //var widget_elems = document.querySelectorAll(".zbase_report_widget");

    //for (var i = 0; i < widget_elems.length; i++) {
    //  if (is_editable) {
    //    widget_elems[i].classList.add("editable");
    //  } else {
    //    widget_elems[i].classList.remove("editable");
    //  }
    //}
  };

  //var renderEditView = function(widget) {
  //  var tpl = $.getTemplate(
  //      "views/report",
  //      "zbase_report_edit_widget_tpl");

  //  var view = widget.editView;

  //  pane.innerHTML = "";
  //  view.render($(".zbase_report_widget_pane", tpl));
  //  pane.appendChild(tpl);
  //};


  return {
    getJSON: getJSON,
    setEditable: setEditable
  }
};

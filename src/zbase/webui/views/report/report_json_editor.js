var ReportJSONEditor = function(json) {
  var render = function(elem) {
    console.log("render json editor", json);
    var tpl = $.getTemplate(
        "views/report",
        "zbase_report_raw_json_editor_tpl");

    // code editor
    editor = $("z-codeeditor", tpl);
    editor.setValue(json);

    elem.appendChild(tpl);

  };

  var destroy = function() {

  };

  return {
    render: render,
    destroy: destroy
  }
};

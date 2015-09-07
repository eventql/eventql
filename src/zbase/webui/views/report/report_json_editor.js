var ReportJSONEditor = function(json) {
  var cancel_callbacks = [];
  var save_callbacks = [];

  var render = function(elem) {
    var tpl = $.getTemplate(
        "views/report",
        "zbase_report_raw_json_editor_tpl");

    // code editor
    var editor = $("z-codeeditor", tpl);
    editor.setValue(json);

    var error_note = $(".error_note", tpl);

    $.onClick($("button.cancel", tpl), triggerCancel);
    $.onClick($("button.submit", tpl), function() {
      var new_json = editor.getValue();
      if (isValidJSON(new_json)) {
        json = new_json;
        triggerSave();
      } else {
        error_note.classList.remove("hidden");
      }
    });

    elem.appendChild(tpl);
  };

  var addCancelCallback = function(callback) {
    cancel_callbacks.push(callback);
  };

  var addSaveCallback = function(callback) {
    save_callbacks.push(callback);
  };

  var triggerCancel = function() {
    cancel_callbacks.forEach(function(callback) {
      callback();
    });
  };

  var triggerSave = function() {
    save_callbacks.forEach(function(callback) {
      callback(json);
    });
  };

  var isValidJSON = function(str) {
    try {
      JSON.parse(str);
      return true;
    } catch (e) {
      return false;
    }
  };

  return {
    render: render,
    destroy: function() {},
    onCancel: addCancelCallback,
    onSave: addSaveCallback
  }
};

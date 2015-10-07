ZBase.registerView((function() {

  var load = function(path) {
    var path_prefix = "/a/logs/";
    var logfile = path.substr(path_prefix.length);

    $.httpGet("/api/v1/logfiles/get_definition?logfile=" + logfile, function(r) {
      if (r.status == 200) {
        render(JSON.parse(r.response));
      } else {
        $.fatalError();
      }
    });
  };

  var render = function(def) {
    var page = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_tpl");

    $("h1", page).innerHTML = def.name;
    $("textarea.regex", page).value = def.regex;

    renderFields(
        $(".editor_pane.source_fields", page),
        def.source_fields,
        editSourceField,
        deleteSourceField);

    renderFields(
        $(".editor_pane.row_fields", page),
        def.row_fields,
        editRowField,
        deleteRowField);

    $.onClick($(".editor_pane.source_fields .add_field", page), addSourceField);
    $.onClick($(".editor_pane.row_fields .add_field", page), addRowField);

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderFields = function(elem, fields, onEdit, onDelete) {
    var tbody = $("tbody", elem);
    var tr_tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_row_tpl");

    fields.forEach(function(field) {
      var tr = tr_tpl.cloneNode(true);
      $(".name", tr).innerHTML = field.name;
      $(".type", tr).innerHTML = field.type;
      $(".format", tr).innerHTML = field.format;

      $("z-dropdown", tr).addEventListener("change", function() {
        switch (this.getValue()) {
          case "edit":
            onEdit(field);
            break;

          case "delete":
            onDelete(field);
            break;
        }
        this.setValue([]);
      }, false);

      tbody.appendChild(tr);
    });
  };

  var editSourceField = function(field) {
    renderEdit(field, "Edit Source Field", function() {
      console.log(field);
    });
  };

  var editRowField = function(field) {
    renderEdit(field, "Edit Row Field", function() {
      console.log(field);
    });
  };

  var addSourceField = function() {
    var field = {};
    renderEdit(field, "Add Source Field", function() {
      console.log(field);
    });
  };

  var addRowField = function() {
    var field = {};
    renderEdit(field, "Add Row Field", function() {
      console.log(field);
    });
  };

  var deleteSourceField = function(field) {
    renderDelete(field.name, "Delete Source Field", function() {
      console.log("delete", field);
    });
  };

  var deleteRowField = function(field) {
    renderDelete(field.name, "Delete Row Field", function() {
      console.log("delete", field);
    });
  };

  var renderEdit = function(def, header, callback) {
    var modal = $(".zbase_logviewer .logfile_editor z-modal.edit_field");
    var tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_modal_tpl");

    $(".z-modal-header", tpl).innerHTML = header;

    var name_input = $("input.name", tpl);
    if (def.name) {
      name_input.value = def.name;
    }

    var type_control = $("z-dropdown", tpl);
    if (def.type) {
      type_control.setValue([def.type]);
    }

    var format_input = $("input.format", tpl)
    if (def.format) {
      format_input.value = def.format;
    }

    $.onClick($("button.close", tpl), function() {
      modal.close();
    });

    $.onClick($("button.submit", tpl), function() {
      if (name_input.value.length == 0) {
        $(".error_note", modal).classList.remove("hidden");
        name_input.classList.add("error");
        return;
      }

      def.name = $.escapeHTML(name_input.value);
      def.type = type_control.getValue();
      def.format = $.escapeHTML(format_input.value);
      callback();
    });


    $.replaceContent($(".container", modal), tpl);
    modal.show();
    name_input.focus();
  };

  var renderDelete = function(field_name, header, callback) {
    var modal =  $(".zbase_logviewer .logfile_editor z-modal.delete_field");
    var tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_delete_modal_tpl");

    $(".z-modal-header", tpl).innerHTML = header;
    $(".field_name", tpl).innerHTML = field_name;

    $.onClick($("button.close", tpl), function() {
      modal.close();
    });
    $.onClick($("button.submit", tpl), callback);

    $.replaceContent($(".container", modal), tpl);
    modal.show();
  };

  return {
    name: "logviewer_logfile_editor",
    loadView: function(params) { load(params.path); },
    unloadView: function() {},
    handleNavigationChange: load
  };
})());

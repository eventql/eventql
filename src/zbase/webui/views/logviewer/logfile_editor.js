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

    renderRegexPane($(".editor_pane.regex", page), def.regex);

    renderFields(
        $(".editor_pane.source_fields", page),
        def.source_fields,
        deleteSourceField);

    renderFields(
        $(".editor_pane.row_fields", page),
        def.row_fields,
        deleteRowField);

    $.onClick($(".editor_pane.source_fields .add_field", page), addSourceField);
    $.onClick($(".editor_pane.row_fields .add_field", page), addRowField);

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderRegexPane = function(elem, regex) {
    var textarea = $("textarea", elem);
    textarea.value = regex;
    $.onClick($("button.submit", elem), function() {
      console.log("save regex", textarea.value);
    });
  };

  var renderFields = function(elem, fields, onDelete) {
    var tbody = $("tbody", elem);
    var tr_tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_row_tpl");

    fields.forEach(function(field) {
      var tr = tr_tpl.cloneNode(true);
      $(".name", tr).innerHTML = field.name;
      $(".type", tr).innerHTML = field.type;
      $(".format", tr).innerHTML = field.format;

      $.onClick($(".delete", tr), function() {onDelete(field);});

      tbody.appendChild(tr);
    });
  };


  var addSourceField = function() {
    renderAdd("Add Source Field", function(field) {
      console.log(field);
    });
  };

  var addRowField = function() {
    renderAdd("Add Row Field", function(field) {
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

  var renderAdd = function(header, callback) {
    var modal = $(".zbase_logviewer .logfile_editor z-modal.edit_field");
    var tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_modal_tpl");

    $(".z-modal-header", tpl).innerHTML = header;

    $.onClick($("button.close", tpl), function() {
      modal.close();
    });

    var name_input = $("input.name", tpl);
    $.onClick($("button.submit", tpl), function() {
      var def = {};
      var error = false;

      //name validation
      def.name = $.escapeHTML(name_input.value);
      if (def.name.length == 0) {
        $(".error_note.name", modal).classList.remove("hidden");
        name_input.classList.add("error");
        name_input.focus();
        error = true;
      } else {
        $(".error_note.name", modal).classList.add("hidden");
        name_input.classList.remove("error");
      }

      var format_input = $("input.format", modal);
      def.format = $.escapeHTML(format_input.value);

      //type format validation
      def.type = $("z-dropdown", modal).getValue();
      if (def.type == "DATETIME" && def.format.length == 0) {
        $(".error_note.date_time_format", modal).classList.remove("hidden");
        format_input.classList.add("error");

        if (!error) {
          format_input.focus();
        }
        error = true;
      } else {
        $(".error_note.date_time_format", modal).classList.add("hidden");
        format_input.classList.remove("error");
      }

      if (!error) {
        callback(def);
      }
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

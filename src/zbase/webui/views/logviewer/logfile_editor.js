ZBase.registerView((function() {
  var logfile;
  var info_message;

  var init = function(path) {
    var path_prefix = "/a/logs/";
    logfile = path.substr(path_prefix.length);

    load();
  }

  var load = function() {
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

    info_message = ZbaseInfoMessage(page);
    $("h1", page).innerHTML = def.name;

    renderRegexPane($(".editor_pane.regex", page), def.regex);

    renderSourceFields(
        $(".editor_pane.source_fields", page),
        def.source_fields);

    renderRowFields(
        $(".editor_pane.row_fields", page),
        def.row_fields);

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderRegexPane = function(elem, regex) {
    var textarea = $("textarea", elem);
    renderRegexContent(textarea, regex);

    var submit_button = $("button.submit", elem);
    var saving_button = $("button.saving", elem);

    $.onClick(submit_button, function() {
      saving_button.classList.remove("hidden");
      submit_button.classList.add("hidden");

      var url =
        "/api/v1/logfiles/set_regex?logfile=" + encodeURIComponent(logfile) +
        "&regex=" + encodeURIComponent(textarea.value);

      $.httpPost(url, "", function(r) {
        if (r.status == 201) {
          saving_button.classList.add("hidden");
          submit_button.classList.remove("hidden");
          info_message.renderSuccess("Your changes have been saved.");
          regex = textarea.value;
        } else {
          info_message.renderError(
              "An error occured, your changes have not been saved.");
          saving_button.classList.add("hidden");
          submit_button.classList.remove("hidden");
        }

        renderRegexContent(textarea, regex);
      });
    });
  };

  var renderRegexContent = function(textarea, content) {
    textarea.value = content;
  };

  var renderSourceFields = function(elem, fields) {
    var onDelete = function(field) {
      renderDelete(field.name, "Delete Source Field", function(def) {
        var url =
            "/api/v1/logfiles/remove_source_field?logfile=" +
            logfile + "&name=" + field.name;

        $.httpPost(url, "", function(r) {
          if (r.status == 201) {
            //remove field from fields
            for (var i = 0; i < fields.length; i++) {
              if (fields[i].name == field.name) {
                fields.splice(i, 1);
                break;
              }
            }

            info_message.renderSuccess("Your changes have been saved.");
            renderFields(elem, fields, onDelete);
          } else {
            info_message.renderError(
              "An error occured, your changes have not been saved.");
          }
        });
      });
    };

    $.onClick($(".add_field", elem), function() {
      renderAdd("Add Source Field", function(field) {
        var url =
            "/api/v1/logfiles/add_source_field?logfile=" +
            logfile + "&" + $.buildQueryString(field);

        $.httpPost(url, "", function(r) {
          if (r.status == 201) {
            fields.push(field);
            info_message.renderSuccess("Your changes have been saved.");
            renderFields(elem, fields, onDelete);
          } else {
            info_message.renderError(
              "An error occured, your changes have not been saved.");
          }
        });

      });
    });

    renderFields(elem, fields, onDelete);
  };

  var renderRowFields = function(elem, fields) {
    var onDelete = function(field) {
      renderDelete(field.name, "Delete Row Field", function(def) {
        var url =
            "/api/v1/logfiles/remove_row_field?logfile=" +
            logfile + "&name=" + field.name;

        $.httpPost(url, "", function(r) {
          if (r.status == 201) {
            //remove field from fields
            for (var i = 0; i < fields.length; i++) {
              if (fields[i].name == field.name) {
                fields.splice(i, 1);
                break;
              }
            }

            info_message.renderSuccess("Your changes have been saved.");
            renderFields(elem, fields, onDelete);
          } else {
            info_message.renderError(
              "An error occured, your changes have not been saved.");
          }
        });
      });
    };

    $.onClick($(".add_field", elem), function() {
      renderAdd("Add Row Field", function(field) {
        var url =
            "/api/v1/logfiles/add_row_field?logfile=" +
            logfile + "&" + $.buildQueryString(field);

        $.httpPost(url, "", function(r) {
          if (r.status == 201) {
            fields.push(field);
            info_message.renderSuccess("Your changes have been saved.");
            renderFields(elem, fields, onDelete);
          } else {
            info_message.renderError(
              "An error occured, your changes have not been saved.");
          }
        });

      });
    });

    renderFields(elem, fields, onDelete);
  };

  var renderFields = function(elem, fields, onDelete) {
    var tbody = $("tbody", elem);
    var tr_tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_row_tpl");

    tbody.innerHTML = "";
    fields.forEach(function(field) {
      var tr = tr_tpl.cloneNode(true);
      $(".name", tr).innerHTML = field.name;
      $(".type", tr).innerHTML = field.type;
      $(".format", tr).innerHTML = field.format;

      $.onClick($(".delete", tr), function() {onDelete(field);});

      tbody.appendChild(tr);
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
        modal.close();
        callback(def);
      }
    });

    $.replaceContent($(".container", modal), tpl);
    modal.show();
    name_input.focus();
  };

  var renderDelete = function(field_name, header, onDelete) {
    var modal =  $(".zbase_logviewer .logfile_editor z-modal.delete_field");
    var tpl = $.getTemplate(
        "views/logviewer",
        "zbase_logviewer_logfile_editor_delete_modal_tpl");

    $(".z-modal-header", tpl).innerHTML = header;
    $(".field_name", tpl).innerHTML = field_name;

    $.onClick($("button.close", tpl), function() {
      modal.close();
    });

    $.onClick($("button.submit", tpl), function() {
      onDelete();
      modal.close();
    });

    $.replaceContent($(".container", modal), tpl);
    modal.show();
  };

  return {
    name: "logviewer_logfile_editor",
    loadView: function(params) { init(params.path); },
    unloadView: function() {},
    handleNavigationChange: init
  };
})());

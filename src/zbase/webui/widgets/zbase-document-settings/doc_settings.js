var DocumentSettingsWidget = function(elem, docid) {
  var init = function() {
    var tpl = $.getTemplate(
        "widgets/zbase-document-settings",
        "zbase_doc_settings_main_tpl");

    //FIXME share_link
    var share_modal = ShareDocModal(
        tpl,
        docid,
        "http://zbase.io" + window.location.pathname);
    elem.appendChild(tpl);

    $.httpGet("/api/v1/documents/" + docid + "?no_content=true", function(r) {
      if (r.status == 200) {
        var doc = JSON.parse(r.response);
        setupStatusUpdate(doc.publishing_status);
        setAclPolicy(doc.acl_policy);
        setCategory(doc.category);
      } else {
        $.fatalError();
      }
    });

    setupCategoryUpdate();
    setupDocumentDeletion();

    share_modal.onUpdate(setAclPolicy);
    $.onClick($(".doc_setting_value.acl_policy", elem), function() {
      share_modal.show();
    });

    $.onClick($(".link.clone", elem), cloneDocument);
  };

  var setupCategoryUpdate = function() {
    var modal = $("z-modal.zbase_doc_settings_modal.category", elem);
    var input = $("input", modal);
    var infobar = $(".doc_settings_infobar", modal);

    $.onClick($(".doc_setting_value.category", elem), function() {
      infobar.classList.add("hidden");
      input.value = this.innerText;
      modal.show();
      input.focus();
    });

    $.onClick($("button.submit", modal), function() {
      var category = input.value;
      var postbody = "category=" + encodeURIComponent(category);
      infobar.innerHTML = "Saving...";
      infobar.classList.remove("hidden");
      $.httpPost("/api/v1/documents/" + docid, postbody, function(r) {
        if (r.status == 201) {
          setCategory(category);
          modal.close();
        } else {
          infobar.innerHTML = "Saving Failed";
        }
      });
    });
  };

  var setupStatusUpdate = function(doc_status) {
    var tpl = $.getTemplate(
        "widgets/zbase-document-settings",
        "zbase_doc_settings_status_inner_tpl");

    var dropdown = $("z-dropdown", tpl);
    $("z-dropdown-item[data-value='" + doc_status + "']", tpl)
        .setAttribute("data-selected", "selected");
    dropdown.setAttribute("data-resolved", "resolved");

    $.replaceContent($(".doc_setting_value.status", elem), tpl);

    dropdown.addEventListener("change", function() {
      var new_status = dropdown.getValue();
      var postbody = "publishing_status=" + encodeURIComponent(new_status);
      //showInfobar
      $.httpPost("/api/v1/documents/" + docid, postbody, function(r) {
        if (r.status == 201) {
          doc_status = new_status;
        } else {
          dropdown.setValue([doc_status]);
          //showInfobar with error
        }
      });
    }, false);
  };

  var setupDocumentDeletion = function() {
    var modal = $("z-modal.zbase_doc_settings_modal.delete_confirmation", elem);
    $.onClick($(".link.delete", elem), function() {modal.show();});
    $.onClick($("button.close", modal), function() {modal.close();});

    $.onClick($("button.submit", modal), function() {
      $.httpPost("/api/v1/documents/" + docid, "deleted=true", function(r) {
        if (r.status == 201) {
          $.navigateTo("/a/");
        } else {
          //show infobar with error
        }
      });
    });
  };

  var cloneDocument = function() {
    alert("Not yet implemented");
  };


  var setAclPolicy = function(acl_policy) {
    if (acl_policy == "ACLPOLICY_PRIVATE") {
      $(".doc_setting_value.acl_policy", elem).innerHTML = "Private"
    } else {
      var config = $.getConfig();
      $(".doc_setting_value.acl_policy", elem).innerHTML =
          "Everybody at " + config.current_user.namespace;
    }
  };

  var setCategory = function(category) {
    if (category.length == 0) {
      $(".doc_setting_value.category", elem).innerHTML = "No Category";
    } else {
      $(".doc_setting_value.category", elem).innerHTML = category;
    }
  };

  return {
    render: init
  }
};

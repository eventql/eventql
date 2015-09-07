var DocumentSettingsWidget = function(elem, docid, share_modal) {
  var init = function() {
    var tpl = $.getTemplate(
        "widgets/zbase-document-settings",
        "zbase_doc_settings_main_tpl");
    elem.appendChild(tpl);

    $.httpGet("/api/v1/documents/" + docid + "?no_content=true", function(r) {
      if (r.status == 200) {
        var doc = JSON.parse(r.response);
        console.log(doc);
        renderStatus(doc.publishing_status);
        setAclPolicy(doc.acl_policy);
        setCategory(doc.category);
      } else {
        $.fatalError();
      }
    });

    initCategoryUpdate();

    //sharing widget
    share_modal.onUpdate(setAclPolicy);
    $.onClick($(".doc_setting_value.acl_policy", elem), function() {
      share_modal.show();
    });

    $.onClick($(".link.clone", elem), function() {
      alert("Not yet implemented");
    });

    $.onClick($(".link.delete", elem), function() {
      //deleted = true
      alert("Not yet implemented");
    });
  };

  var initCategoryUpdate = function() {
    var modal = $("z-modal.zbase_doc_settings_modal", elem);
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

  var renderStatus = function(doc_status) {
    var tpl = $.getTemplate(
        "widgets/zbase-document-settings",
        "zbase_doc_settings_status_inner_tpl");

    var dropdown = $("z-dropdown", tpl);
    $("z-dropdown-item[data-value='" + doc_status + "']", tpl)
        .setAttribute("data-selected", "selected");
    dropdown.setAttribute("data-resolved", "resolved");

    $.replaceContent($(".doc_setting_value.status", elem), tpl);

    dropdown.addEventListener("change", function() {
      updateSettings("publishing_status=" + encodeURIComponent(dropdown.getValue()));
    }, false);
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

  var updateSettings = function(postbody) {
    //showInfobar
    $.httpPost("/api/v1/documents/" + docid, postbody, function(r) {
      if (r.status == 201) {
        //showInfobar
      } else {
        //showInfobar with error
      }
    });
  };


  return {
    render: init
  }
};

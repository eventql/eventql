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
        renderAclPolicy(doc.acl_policy);
        renderCategory(doc.category);
      } else {
        $.fatalError();
      }
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

  var renderAclPolicy = function(acl_policy) {
    //FIXME share link
    share_modal.onUpdate(setAclPolicy);
    $.onClick($(".doc_setting_value.acl_policy", elem), function() {
      share_modal.show();
    });

    setAclPolicy(acl_policy);
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


  var renderCategory = function(category) {
    $(".doc_setting_value.category", elem).innerHTML = category;
  }

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

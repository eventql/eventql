var DocumentSettingsWidget = function(elem, docid) {
  var init = function() {
    var tpl = $.getTemplate(
        "widgets/zbase-document-settings",
        "zbase_doc_settings_main_tpl");
    elem.appendChild(tpl);

    $.httpGet("/api/v1/documents/" + docid + "?no_content=true", function(r) {
      if (r.status == 200) {
        var doc = JSON.parse(r.response);
        console.log(doc);
        setStatus(doc.publishing_status);
        setAclPolicy(doc.acl_policy);
        setCategory(doc.category);
      } else {
        $.fatalError();
      }
    });
  };

  var setStatus = function(doc_status) {
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
    var value = $(".doc_setting_value.acl_policy", elem);
    if (acl_policy == "ACLPOLICY_PRIVATE") {
      value.innerHTML = "Private"
    } else {
      var config = $.getConfig();
      value.innerHTML = "Everybody at " + config.current_user.namespace;
    }
  };

  var setCategory = function(category) {
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

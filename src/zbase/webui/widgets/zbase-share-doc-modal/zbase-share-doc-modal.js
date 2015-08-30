var ShareDocModal = function(elem, id, link) {
  var showModal = function() {
    loadSettings();
    modal.show();
  };

  var loadSettings = function() {
    $.httpGet("/api/v1/documents/" + doc_id + "?no_content=true", function(r) {
      if (r.status == 200) {
        var response = JSON.parse(r.response);
        $(".zbase_share_doc_link", modal).value = share_link;
        renderACLPolicy(response.acl_policy);
        renderNamespaceInfo();
      } else {
        $.fatalError();
      }
    });
  };

  var getACLPolicy = function() {
    return $(".access_selection[data-selected]", modal).getAttribute("data-policy");
  };

  var renderACLPolicy = function(acl_policy) {
    var selection = $(".access_selection[data-selected]", modal);
    if (selection) {
      selection.removeAttribute("data-selected");
    }

    $(".access_selection[data-policy='" + acl_policy + "']", modal)
        .setAttribute("data-selected", "selected");
  };

  var renderNamespaceInfo = function() {
    var config = $.getConfig();

    var info_elems = modal.querySelectorAll(".namespace_info");
    for (var i = 0; i < info_elems.length; i++) {
      info_elems[i].innerHTML = config.current_user.namespace;
    }
  };

  var updateSettings = function() {
    var infobar = $(".share_doc_infobar", modal);
    infobar.innerHTML = "Saving...";
    infobar.classList.remove("hidden");

    var post_body = "acl_policy=" + encodeURIComponent(getACLPolicy());
    $.httpPost("/api/v1/documents/" + doc_id, post_body, function(r) {
      if (r.status == 201) {
        hideModal();
        infobar.classList.add("hidden");
      } else {
        infobar.innerHTML = "Saving Failed";
      }
    });
  };

  var hideModal = function() {
    modal.close();
  };

  var doc_id = id;
  var share_link = link;
  var tpl = $.getTemplate(
    "widgets/zbase-share-doc-modal",
    "zbase-share-doc-modal-main-tpl");
  var modal = $("z-modal", tpl);
  elem.appendChild(tpl);

  // submit button
  $.onClick($("button[data-action='submit']", modal), function() {
    updateSettings();
  });

  var access_selections = modal.querySelectorAll(".access_selection");
  for (var i = 0; i < access_selections.length; i++) {
    $.onClick(access_selections[i], function() {
      renderACLPolicy(this.getAttribute("data-policy"));
    });
  }

  return {
    show: showModal,
    hide: hideModal
  }
};

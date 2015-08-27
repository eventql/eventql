var ShareDocModal = function(elem) {
  var showModal = function(id, doc_url) {
    doc_id = id;
    loadSettings(doc_url);
    modal.show();
  };

  var loadSettings = function(doc_url) {
    $.httpGet("/api/v1/documents/" + doc_id + "?no_content=true", function(r) {
      if (r.status == 200) {
        var response = JSON.parse(r.response);
        $(".zbase_share_doc_link", modal).value = doc_url;
        setAccessSelection(response.acl_policy);
        //$.hideLoader();
      } else {
        $.fatalError();
      }
    });
  };


  var getAccessSelection = function() {
    return $(".access_selection[data-selected]", modal).getAttribute("data-policy");
  };

  var setAccessSelection = function(acl_policy) {
    var selection = $(".access_selection[data-selected]", modal);
    if (selection) {
      selection.removeAttribute("data-selected");
    }

    $(".access_selection[data-policy='" + acl_policy + "']", modal)
        .setAttribute("data-selected", "selected");
  };

  var updateSettings = function() {
    var post_body = "acl_policy=" + encodeURIComponent(getAccessSelection());

  };

  var hideModal = function() {
    modal.hide();
  };


  var doc_id;
  var tpl = $.getTemplate(
    "widgets/zbase-share-doc-modal",
    "zbase-share-doc-modal-main-tpl");
  var modal = $("z-modal", tpl);
  elem.appendChild(tpl);

  // submit button
  $.onClick($("button[data-action='submit']", modal), function() {
    updateSettings();
  });

  return {
    show: showModal,
    hide: hideModal
  }
};

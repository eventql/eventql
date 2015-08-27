var ShareDocModal = function(elem) {
  var showModal = function() {

  };

  var hideModal = function() {

  };


  var tpl = $.getTemplate(
    "widgets/zbase-share-doc-modal",
    "zbase-share-doc-modal-main-tpl");
  var modal = $("z-modal", tpl);
  elem.appendChild(tpl);

  return {
    show: showModal,
    hide: hideModal
  }
};

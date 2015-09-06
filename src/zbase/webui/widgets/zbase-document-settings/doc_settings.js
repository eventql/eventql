var DocumentSettingsWidget = function(docid) {
  var render = function(elem) {
    var tpl = $.getTemplate(
        "widgets/zbase-document-settings",
        "zbase_doc_settings_main_tpl");

    elem.appendChild(tpl);
    //$.httpGet("/api/v1/documents/" + docid + "?no_content=true", function(r) {
    console.log(docid);
    console.log(elem);
  };

  return {
    render: render
  }
};

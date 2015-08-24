var QueryProgressWidget = (function() {

  var render = function(elem, progress) {
    var tpl = $.getTemplate(
        "widgets/zbase-query-progress",
        "zbase_query_progress_main_tpl");

    elem.innerHTML = "";
    elem.appendChild(tpl)
  };

  return {
    render: render
  };

})();

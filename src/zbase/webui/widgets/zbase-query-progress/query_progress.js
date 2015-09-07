var QueryProgressWidget = (function() {

  var render = function(elem, data) {
    var tpl = $.getTemplate(
        "widgets/zbase-query-progress",
        "zbase_query_progress_main_tpl");

    var pbar = $("z-progressbar", tpl);
    if (data && data.status == "running") {
      pbar.setAttribute("data-progress", (data.progress * 100));
      pbar.setAttribute("data-label", data.message);
    } else {
      pbar.setAttribute("data-label", "Waiting...");
    }

    //if (waiting_for_long_time_already) {
    //  var header = loader.querySelector("h5");
    //  if (header) {
    //    header.style.visibility = "visible";
    //  }
    //}

    elem.innerHTML = "";
    elem.appendChild(tpl)
  };

  return {
    render: render
  };

})();

var QueryProgressWidget = (function() {
  var pbar;

  var render = function(elem, data) {
    var tpl = $.getTemplate(
        "widgets/zbase-query-progress",
        "zbase_query_progress_main_tpl");

    pbar = $("z-progressbar", tpl);
    if (data && data.status == "running") {
      if (data.progress) {
        pbar.setAttribute("data-progress", (data.progress * 100));
      }

      if (data.message) {
        pbar.setAttribute("data-label", data.message);
      } else {
        pbar.setAttribute("data-label", "Running...");
      }
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

  var finish = function(callback) {
    if (!pbar) {
      callback();
    }

    pbar.setAttribute("data-label", "Running...");
    var progress = parseInt(pbar.getAttribute("data-progress"), 10);
    if (isNaN(progress)) {
      progress = 0;
    }
    var interval_id = window.setInterval(function() {
      progress += 5;

      if (progress == 100) {
        window.clearInterval(interval_id);
        callback();
      }
      pbar.setAttribute("data-progress", progress);
    }, 20);
  };

  return {
    render: render,
    finish: finish
  };

})();

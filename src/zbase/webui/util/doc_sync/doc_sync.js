var DocSync = function(getPostBody, save_url, infobar_elem) {
  var cur_version = 0;
  var retry_delay = 5000;

  var state = "idle";
  var sync_version = 0;
  var last_failure = null;
  var running_timer = null;

  var saveDocument = function() {
    cur_version++;
    documentChanged("content_changed");
  }

  var documentChanged = function(cur_event) {
    switch (state) {

      case "idle":
        switch (cur_event) {

          case "content_changed":
            syncToBackend();
            return;

        }
        break;

      case "saving":
        switch (cur_event) {

          case "saved_success":
            if (sync_version == cur_version) {
              state = "idle";
              hideInfobar();
              return;
            } else {
              syncToBackend();
              return;
            }

          case "saved_failure":
            last_failure = (new Date()).getTime();
            state = "retry";
            documentChanged(null);
            return;

        }
        break;

      case "retry":
        switch (cur_event) {

          case "content_changed":
            cancelTimer();
            syncToBackend();
            return;

          default:
            var now = (new Date()).getTime();

            if (last_failure + retry_delay > now) {
              var retry_in = Math.ceil((last_failure + retry_delay - now) / 1000);
              updateInfobar("Saving Failed! Retrying in " + retry_in + " seconds");
              showInfobar();
              runTimer(function() { documentChanged(null); }, 300);
            } else {
              syncToBackend();
            }

            return;

        }
        break;

    }

  }

  var syncToBackend = function() {
    var syncing_version = cur_version;
    updateInfobar("Saving...");

    state = "saving";

    var http = new XMLHttpRequest();
    http.open("POST", save_url, true);
    http.timeout = 4000;
    http.send($.buildQueryString(getPostBody()));

    http.onreadystatechange = function() {
      if (http.readyState == 4) {
        if (http.status == 201) {
          sync_version = syncing_version;
          return documentChanged("saved_success");
        } else {
          return documentChanged("saved_failure");
        }
      }
    };

    // FIXME?
    //http.ontimeout = function() {
    //  return onStatusChange("saved_failure");
    //};
  };

  var cancelTimer = function() {
    if (running_timer) {
      window.clearTimeout(running_timer);
    }
  }

  var runTimer = function(fn, timeout) {
    if (running_timer) {
      window.clearTimeout(running_timer);
    }

    running_timer = window.setTimeout(function() {
      running_timer = null;
      fn();
    }, timeout);
  };

  var updateInfobar = function(text) {
    infobar_elem.innerHTML = text;
  };

  var showInfobar = function() {
    infobar_elem.classList.remove('hidden');
  };

  var hideInfobar = function() {
    infobar_elem.classList.add('hidden');
  };

  var close = function() {
    if (running_timer) {
      window.clearTimeout(running_timer);
    }
  }

  return {
    saveDocument: saveDocument,
    close: close
  }
};

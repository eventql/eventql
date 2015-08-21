var DocSync = function(getPostBody, save_url) {
  this.cur_version = 0;

  var retry_delay = 5000;

  var state = "idle";
  var sync_version = 0;
  var last_failure = null;
  var running_timer = null;

  this.documentChanged = function(cur_event) {
    switch (state) {

      case "idle":
        switch (cur_event) {

          case "content_changed":
            this.syncToBackend();
            return;

        }
        break;

      case "saving":
        switch (cur_event) {

          case "saved_success":
            if (sync_version == this.cur_version) {
              state = "idle";
              hideInfobar();
              return;
            } else {
              this.syncToBackend();
              return;
            }

          case "saved_failure":
            last_failure = (new Date()).getTime();
            state = "retry";
            this.documentChanged(null);
            return;

        }
        break;

      case "retry":
        switch (cur_event) {

          case "content_changed":
            cancelTimer();
            this.syncToBackend();
            return;

          default:
            var now = (new Date()).getTime();

            if (last_failure + retry_delay > now) {
              var retry_in = Math.ceil((last_failure + retry_delay - now) / 1000);
              var _this = this;
              updateInfobar("Saving Failed! Retrying in " + retry_in + " seconds");
              showInfobar();
              runTimer(function() { _this.documentChanged(null); }, 300);
            } else {
              this.syncToBackend();
            }

            return;

        }
        break;

    }

  }

  this.syncToBackend = function() {
    var _this = this;
    var syncing_version = this.cur_version;
    updateInfobar("Saving...");

    state = "saving";

    var http = new XMLHttpRequest();
    http.open("POST", save_url, true);
    http.timeout = 4000;

    http.send(getPostBody());

    http.onreadystatechange = function() {
      if (http.readyState == 4) {
        if (http.status == 201) {
          sync_version = syncing_version;
          return _this.documentChanged("saved_success");
        } else {
          return _this.documentChanged("saved_failure");
        }
      }
    };

    // FIXME?
    //http.ontimeout = function() {
    //  return onStatusChange("saved_failure");
    //};
  };

  function cancelTimer() {
    if (running_timer) {
      window.clearTimeout(running_timer);
    }
  }

  function runTimer(fn, timeout) {
    if (running_timer) {
      window.clearTimeout(running_timer);
    }

    running_timer = window.setTimeout(function() {
      running_timer = null;
      fn();
    }, timeout);
  };

  function updateInfobar(text) {
    var infobar = document.querySelector(".infobar");
    infobar.innerHTML = text;
  };

  function showInfobar() {
    document.querySelector(".infobar").classList.remove('hidden');
  };

  function hideInfobar() {
    document.querySelector(".infobar").classList.add('hidden');
  };


};

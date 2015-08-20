var ZBase = (function() {
  var modules_status = {};
  var modules_waitlist = [];
  var config;

  var findRoute = function(path) {
    for (var i = 0; i < config.routes.length; i++) {
      if (path.indexOf(config.routes[i].path_prefix) == 0) {
        return config_routes[i];
      }
    }

    return null;
  };

  var onNavigationChange = function(path) {
    var route = findRoute(path);
    if (route == null) {
      alert("Not Found");
      return;
    }


  };

  var init = function(_config) {
    config = _config;
    onNavigationChange(window.location.pathname + window.location.search);
  };

  var finishModuleDownload = function(module) {
    modules_status[module] = "loaded";
    // fire all finished callbacks
    for (var j = modules_waitlist.length - 1; j >= 0; j--) {
      var entry = modules_waitlist[j];
      for (var i = 0; i < entry.modules.length; i++) {
        if (modules_status[entry.modules[i]] != "loaded") {
          return;
        }
      }

      modules_waitlist.splice(j, 1);
      entry.on_loaded();
    }
  };

  var startModuleDownload = function(module) {
    modules_status[module] = "loading";
    console.log("Loading Module: ", module);

    window.setTimeout(function() {
      finishModuleDownload(module);
    }, 0);
  };

  var loadModules = function(modules, on_loaded) {
    var unloaded_modules = [];

    // search for missing modules
    modules.forEach(function(module) {
      if (modules_status[module] != "loaded") {
        unloaded_modules.push(module);
      }
    });

    // early exit if all modules are already loded
    if (unloaded_modules.length == 0) {
      on_loaded();
      return;
    }

    // add ourselves to the waitlist
    modules_waitlist.push({
      modules: modules,
      on_loaded: on_loaded
    });

    // start loading missing modules
    unloaded_modules.forEach(function(module) {
      if (modules_status[module] != "loading") {
        startModuleDownload(module);
      }
    });
  };


  return {
    init: init,
    loadModules: loadModules
  };
})();


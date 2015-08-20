var ZBase = (function() {
  var current_path;
  var current_route;
  var current_view;
  var modules_status = {};
  var modules_waitlist = [];
  var views = {};
  var config;

  var registerPopstateHandler = function() {
    window.onpopstate = function(e) {
      e.preventDefault();
      if (e.state && e.state.path) {
        changeNavigation(e.state.path);
      } else {
        changeNavigation(window.location.pathname + window.location.search);
      }
    };
  };

  var findRoute = function(path) {
    for (var i = 0; i < config.routes.length; i++) {
      if (path.indexOf(config.routes[i].path_prefix) == 0) {
        return config.routes[i];
      }
    }

    return null;
  };

  var showFatalError = function() {
    alert("Fatal Error, please reload the page");
  };

  var applyNavigationChange = function() {
    if (current_view && current_view.name == current_route.view) {
      current_view.handleNavigationChange(current_path);
      hideLoader();
      return;
    }

    if (current_view) {
      current_view.unloadView();
    }

    current_view = views[current_route.view];
    if (!current_view) {
      showFatalError();
      return;
    }

    current_view.loadView({path: current_path, config: config});
    hideLoader();
  };

  var changeNavigation = function(path) {
    console.log(">> Navigate to: ", path);
    showLoader();

    var route = findRoute(path);
    if (route == null) {
      window.location.href = config.default_route;
      return;
    }

    current_route = route;
    current_path = path;

    loadModules(route.modules, function() {
      applyNavigationChange();
    });
  };

  var navigateTo = function(path) {
    history.pushState({path: path}, "", path);
    changeNavigation(path);
  };

  var init = function(_config) {
    config = _config;
    changeNavigation(window.location.pathname + window.location.search);
    registerPopstateHandler();
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

  var startModulesDownload = function(modules) {
    modules.forEach(function(module) {
      console.log(">> Loading module: ", module);
      modules_status[module] = "loading";

      window.setTimeout(function() {
        var link = document.createElement('link');
        link.rel = 'import';
        link.href = "/modules/" + module + ".html";
        link.onerror = function(e) {
          console.log(">> Error while loading module >" + module + "<, aborting");
          showFatalError();
        };

        document.body.appendChild(link);
      }, 0);
    });
  };

  var loadModules = function(modules, on_loaded) {
    // search for modules that have not finished loading
    var unloaded_modules = [];
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

    // search for modules that haven't started downloading yet
    var download_modules = [];
    unloaded_modules.forEach(function(module) {
      if (modules_status[module] != "loading") {
        download_modules.push(module);
      }
    });

    // start downloading missing modules
    startModulesDownload(download_modules);
  };

  var registerView = function(view) {
    views[view.name] = view;
  };

  var showLoader = function() {
    document.getElementById("zbase_main_loader").classList.remove("hidden");
  };

  var hideLoader = function() {
    document.getElementById("zbase_main_loader").classList.add("hidden");
  };


  return {
    init: init,
    loadModules: loadModules,
    moduleReady: finishModuleDownload,
    registerView: registerView,
    navigateTo: navigateTo
  };
})();


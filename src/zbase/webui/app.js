var ZBase = (function() {
  var fatal_error = false;
  var current_path;
  var current_route;
  var current_view;
  var modules_status = {};
  var modules_waitlist = [];
  var views = {};
  var config;

  /* feature detection */
  var __enable_html5_import = 'import' in document.createElement('link');
  var __enable_html5_templates = ("content" in document.createElement("template"));
  var __enable_html5_importnode = 'importNode' in document;

  try {
    document.importNode(document.createElement('div'));
  } catch (e) {
    __enable_html5_importnode = false;
  }

  var init = function(_config) {
    config = _config;

    console.log(
      ">> Initializing ZBase UI, detected features: " +
      "html5_templates=" + (__enable_html5_templates ? "yes" : "no") + ", " +
      "html5_imports=" + (__enable_html5_import ? "yes" : "no") + ", " +
      "html5_importnode=" + (__enable_html5_importnode ? "yes" : "no"));

    registerPopstateHandler();
    changeNavigation(window.location.pathname + window.location.search);
    renderLayout();
  };

  var getConfig = function() {
    return config;
  };

  var updateConfig = function(new_config) {
    config = new_config;

    renderLayout();
  };

  var showFatalError = function(msg) {
    console.log(">> FATAL ERROR: " + msg);

    if (fatal_error) {
      return;
    }

    showLoader();

    var error_elem = document.createElement("div");
    error_elem.classList.add("zbase_fatal_error");
    error_elem.innerHTML =
        "<span>" +
        "<h1>We're sorry</h1>" +
        "Something went wrong and ZenBase crashed &mdash; please reload the " +
        "page or contact support if the problem persists." +
        "<a href='/a/'>Reload</a>"
        "</span>";

    document.body.appendChild(error_elem);
    fatal_error = true;
  };

  var showLoader = function() {
    document.getElementById("zbase_main_loader").classList.remove("hidden");
  };

  var hideLoader = function() {
    document.getElementById("zbase_main_loader").classList.add("hidden");
  };

  var registerView = function(view) {
    views[view.name] = view;
  };

  var findRoute = function(path) {
    for (var i = 0; i < config.routes.length; i++) {
      if (config.routes[i].path_prefix &&
          path.indexOf(config.routes[i].path_prefix) == 0) {
        return config.routes[i];
      }

      if (config.routes[i].path_match == path) {
        return config.routes[i];
      }
    }

    return null;
  };

  /**
   * Navigation
   */
  var registerPopstateHandler = function() {
    window.addEventListener('load', function() {
      setTimeout(function() {
        window.addEventListener('popstate', function(e) {
          e.preventDefault();
          if (e.state && e.state.path) {
            changeNavigation(e.state.path);
          } else {
            changeNavigation(window.location.pathname + window.location.search);
          }
        }, false);
      }, 0);
    }, false);
  };

  var applyNavigationChange = function() {
    if (current_view && current_view.name == current_route.view) {
      hideLoader();
      current_view.handleNavigationChange(current_path);
      return;
    }

    if (current_view) {
      current_view.unloadView();
      document.getElementById("zbase_viewport").innerHTML = "";
    }

    current_view = views[current_route.view];
    if (!current_view) {
      showFatalError("view not found: " + current_route.view);
      return;
    }

    hideLoader();
    current_view.loadView({path: current_path, config: config});
  };

  var changeNavigation = function(path) {
    console.log(">> Navigate to: ", path);
    showLoader();

    var route = findRoute(path);
    if (route == null) {
      if (path.indexOf("/a/") == 0) {
        navigateTo(config.default_route);
      } else {
        window.location.href = path;
      }
      return;
    }

    current_route = route;
    current_path = path;

    loadModules(route.modules, function() {
      applyNavigationChange();
    });
  };

  var navigateTo = function(path) {
    console.log(">> Navigate to called ", path);
    history.pushState({path: path}, "", path);
    changeNavigation(path);
  };

  /**
   * Module loading
   */
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
        var import_url = "/a/_/m/" + module;

        if (__enable_html5_import) {
          var link = document.createElement('link');
          link.rel = 'import';
          link.href = import_url;
          link.setAttribute("data-module", module);
          link.setAttribute("async", "async");
          link.onerror = function(e) {
            showFatalError("Error while loading module: " + module);
          };

          document.body.appendChild(link);
        } else {
          $.httpGet(import_url, function(http) {
            if (http.status == 200) {
              var dummy = document.createElement("div");
              dummy.innerHTML = http.responseText;
              dummy.style.display = "none";
              document.body.appendChild(dummy);

              var scripts = dummy.getElementsByTagName('script');
              for (var i = 0; i < scripts.length; i++) {
                var script = document.createElement('script');
                script.type = scripts[i].type;
                if (scripts[i].src) {
                  script.src = scripts[i].src;
                } else {
                  script.innerHTML = scripts[i].innerHTML;
                }

                document.head.appendChild(script);
              }
            } else {
              showFatalError("Error while loading module: " + module);
              return;
            }
          });
        }
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

  function importNodeFallback(node, deep) {
    var a, i, il, doc = document;

    switch (node.nodeType) {

      case document.DOCUMENT_FRAGMENT_NODE:
        var new_node = document.createDocumentFragment();
        while (child = node.firstChild) {
          new_node.appendChild(node);
        }
        return new_node;

      case document.ELEMENT_NODE:
        var new_node = doc.createElementNS(node.namespaceURI, node.nodeName);
        if (node.attributes && node.attributes.length > 0) {
          for (i = 0, il = node.attributes.length; i < il; i++) {
            a = node.attributes[i];
            try {
              new_node.setAttributeNS(
                  a.namespaceURI,
                  a.nodeName,
                  node.getAttribute(a.nodeName));
            } catch (err) {}
          }
        }
        if (deep && node.childNodes && node.childNodes.length > 0) {
          for (i = 0, il = node.childNodes.length; i < il; i++) {
            new_node.appendChild(
                importNodeFallback(node.childNodes[i],
                deep));
          }
        }
        return new_node;

      case document.TEXT_NODE:
      case document.CDATA_SECTION_NODE:
      case document.COMMENT_NODE:
        return doc.createTextNode(node.nodeValue);

    }
  }

  var getTemplate = function(module, template_id) {
    var template_selector = "#" + template_id;

    var template = document.querySelector(template_selector);

    if (!template && __enable_html5_import) {
      var template_import = document.querySelector(
          "link[data-module='" + module + "']");

      if (!template_import) {
        return null;
      }

      template = template_import.import.querySelector(template_selector);
    }

    if (!template && __enable_html5_import) {
      var template_imports = document.querySelectorAll("link[rel='import']");

      for (var i = 0; !template && i < template_imports.length; ++i) {
        template = template_imports[i].import.querySelector(template_selector);
      }
    }

    if (!template) {
      return null;
    }

    var content;
    if (__enable_html5_templates) {
      content = template.content;
    } else {
      content = document.createDocumentFragment();
      var children = template.children;

      for (var j = 0; j < children.length; j++) {
        content.appendChild(children[j].cloneNode(true));
      }
    }

    if (__enable_html5_importnode) {
      return document.importNode(content, true);
    } else {
      return importNodeFallback(content, true);
    }
  };

  var renderLayout = function() {
    var conf = $.getConfig();

    // viewport min height
    document.getElementById("zbase_viewport").style.minHeight =
        window.innerHeight + "px";

    // render footer
    document.getElementById("zbase_build_id").innerHTML = conf.zbase_build_id;
    document.getElementById("zbase_domain").innerHTML = conf.zbase_domain;
    document.getElementById("zbase_footer").classList.remove("hidden");

    // render header
    if (conf.current_user) {
      ZBase.loadModules(["widgets/zbase-header"], function() {
        HeaderWidget.render();
      });
    } else {
      var elem = document.querySelector("#zbase_header");
      elem.classList.remove("hidden");
      elem.innerHTML = "";
      elem.appendChild($.getTemplate("", "zbase_header_default_tpl"))
      $.handleLinks(elem);
    }
  };

  return {
    init: init,
    loadModules: loadModules,
    moduleReady: finishModuleDownload,
    registerView: registerView,
    navigateTo: navigateTo,
    getConfig: getConfig,
    updateConfig: updateConfig,
    getTemplate: getTemplate,
    fatalError: showFatalError,
    showLoader: showLoader,
    hideLoader: hideLoader
  };
})();

var $ = function(selector, elem) {
  if (!elem) {
    elem = document;
  }

  return elem.querySelector(selector);
};

$.navigateTo = ZBase.navigateTo;
$.getConfig = ZBase.getConfig;
$.getTemplate = ZBase.getTemplate;
$.fatalError = ZBase.showFatalError;
$.showLoader = ZBase.showLoader;
$.hideLoader = ZBase.hideLoader;

$.handleLinks = function(elem) {
  var click_fn = (function() {
    return function(e) {
      var href = this.getAttribute("href");

      if (href.indexOf("/a/") == 0) {
        $.navigateTo(href);
        e.preventDefault();
        return false;
      } else {
        return true;
      }
    };
  })();

  var elems = elem.querySelectorAll("a");
  for (var i = 0; i < elems.length; ++i) {
    elems[i].addEventListener("click", click_fn);
  }
};

$.onClick = function(elem, fn) {
  elem.addEventListener("click", function(e) {
    e.preventDefault();
    fn();
    return false;
  });
};

$.httpPost = function(url, request, callback) {
  var http = new XMLHttpRequest();
  http.open("POST", url, true);
  var start = (new Date()).getTime();
  http.send(request);

  http.onreadystatechange = function() {
    if (http.readyState == 4) {
      var end = (new Date()).getTime();
      var duration = end - start;
      callback(http, duration);
    }
  }
};

$.httpGet = function(url, callback) {
  var http = new XMLHttpRequest();
  http.open("GET", url, true);
  http.send();

  var base = this;
  http.onreadystatechange = function() {
    if (http.readyState == 4) {
      callback(http);
    }
  }
};

$.buildQueryString = function(params) {
  var qs = "";

  for (var key in params) {
    var value = params[key];
    qs += encodeURIComponent(key) + "=" + encodeURIComponent(value) + "&";
  }

  return qs;
}

$.replaceViewport = function(new_content) {
  var viewport = document.getElementById("zbase_viewport");
  viewport.innerHTML = "";
  viewport.appendChild(new_content);
}

$.replaceContent = function(elem, new_content) {
  elem.innerHTML = "";
  elem.appendChild(new_content);
}

document.getTemplateByID = function(template_name) {
  return $.getTemplate("", template_name);
};

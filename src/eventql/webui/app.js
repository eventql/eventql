/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
EventQL = this.EventQL || {};
EventQL.config = EventQL.config || {};
EventQL.views = (EventQL.views || {});

EventQL = (function() {
  'use strict';

  const VERSION = "v0.4.1";

  var app = this;
  var current_path;
  var router;
  var viewport_elem;
  var current_view;
  var navbar;
  var api_stubs = {};

  var init = function() {
    console.log(">> EventQL Cloud ", VERSION);

    api_stubs.evql = new API({});

    navbar = new EventQL.Navbar(document.querySelector(".navbar"));
    navbar.render();

    document.querySelector(".navbar").style.display = "block";
    showLoader();

    viewport_elem = document.getElementById("evql_viewport");
    setPath(window.location.pathname + window.location.search);

    /* handle history entry change */
    setTimeout(function() {
      window.addEventListener('popstate', function(e) {
        e.preventDefault();
        if (e.state && e.state.path) {
          applyNavgiationChange(e.state.path);
        } else {
          setPath(window.location.pathname + window.location.search);
        }
      }, false);
    }, 0);
  }

  var navigateTo = function(url) {
    var path = URLUtil.getPathAndQuery(url);
    history.pushState({path: path}, "", path);
    setPath(path);
  }

  var navigateHome = function() {
    navigateTo("/");
  }

  var showLoader = function() {
    //FIXME
    //document.getElementById("evql_main_loader").classList.remove("hidden");
  };

  var hideLoader = function() {
    //FIXME
    //document.getElementById("evql_main_loader").classList.add("hidden");
  };

  /** PRIVATE **/
  function setDatabase(db) {
    api_stubs.evql.setDatabase(db);
    navbar.setDatabase(db);
  }

  function setPath(path) {
    if (path == current_path) {
      return;
    }

    current_path = path;
    console.log(">> Navigate to: " + path);

    var params = {};
    params.app = app;
    params.path = path;

    var m = path.match(/\/ui\/([a-z0-9A-Z_-]+)(\/?.*)/);
    if (m) {
      setDatabase(m[1]);
      params.vpath = "/ui/<database>" + m[2];
    } else {
      params.vpath = path;
    }

    /* try changing the path in the current view instance */
    if (current_view && current_view.changePath) {
      var route = findRoute(params.vpath);
      if (current_view.changePath(path, route)) {
        return;
      }
    }

    /* otherwise search for the new view class */
    showLoader();

    var route = params.path ? findRoute(params.vpath) : null;
    params.route = route;

    var view = route ? EventQL.views[route.view] : null;
    if (!view) {
      view = EventQL.views["eventql.error"];
    }

    /* destroy the current view */
    if (current_view && current_view.destroy) {
      current_view.destroy();
    }

    /* initialize the new view */
    viewport_elem.innerHTML = "";
    current_view = {};

    view.call(current_view, viewport_elem, params);

    if (current_view.initialize) {
      current_view.initialize.call(current_view);
    }

    hideLoader();
  }

  function findRoute(full_path) {
    var path = full_path;
    var end = path.indexOf("?");
    if (end >= 0) {
      path = path.substring(0, end)
    }

    var routes = EventQL.routes;
    for (var i = 0; i < routes.length; i++) {
      if (routes[i].route instanceof RegExp) {
        var match = path.match(routes[i].route);
        if (match) {
          var route = {};
          for (var k in routes[i]) {
            route[k] = routes[i][k];
          }

          route.args = {}
          for (var j = 1; j < match.length; ++j) {
            route.args[route.route_args[j - 1]] = match[j];
          }

          return route;
        }
      } else {
        if (path === routes[i].route) {
          return routes[i];
        }
      }
    }

    return null;
  }

  this["init"] = init;
  this.navigateTo = navigateTo;
  this.api_stubs = api_stubs;
  this.version = VERSION;
  return this;
}).apply(EventQL);

EventQL.util = EventQL.util || {};
EventQL.util.rewriteLinks = function(elem, database) {
  var elems = elem.querySelectorAll("a");
  for (var i = 0; i < elems.length; ++i) {
    elems[i].href = elems[i].href.replace("%database%", database);
  }
};

EventQL.util.rewriteAndHandleLinks = function(elem, database) {
  EventQL.util.rewriteLinks(elem, database);
  DOMUtil.handleLinks(elem, EventQL.navigateTo);
}


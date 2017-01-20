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

URLUtil = this.URLUtil || {};

URLUtil.getParamValue = function(url, key) {
  var a = document.createElement('a');
  a.href = url;

  var query_string = a.search.substr(1);
  var key = encodeURIComponent(key) + "=";
  var params = query_string.split("&");
  for (var i = 0; i < params.length; i++) {
    if (params[i].lastIndexOf(key, 0) > -1) {
      return decodeURIComponent(params[i].substr(key.length));
    }
  }

  return null;
};

URLUtil.addOrModifyParam = function(url, param, new_value) {
  var a = document.createElement('a');
  a.href = url;

  var vars = a.search.substr(1).split("&");
  var new_vars = "";
  var found = false;

  for (var i = 0; i < vars.length; i++) {
    if (vars[i].length == 0) {
      continue;
    }

    var pair = vars[i].split('=', 2);
    var val = pair[1];
    if (pair[0] == param) {
      val = new_value;
      found = true;
    }

    new_vars += pair[0] + "=" + val + "&";
  }

  if (!found) {
    new_vars += param + "=" + new_value;
  }

  a.search = "?" + new_vars;
  return a.href;
}

URLUtil.comparePaths = function(a, b) {
  var a_path = URLUtil.getPath(a);
  var b_path = URLUtil.getPath(b);
  var a_params = URLUtil.getParams(a);
  var b_params = URLUtil.getParams(b);

  var params_changed =  {};
  var keys = Object.keys(a_params).concat(Object.keys(b_params));
  for (var i = 0; i < keys.length; i++) {
    if (keys[i].length == 0) {
      continue;
    }

    if (a_params[keys[i]] != b_params[keys[i]]) {
      params_changed[keys[i]] = true;
    }
  }

  return {
    path: a_path != b_path,
    params: Object.keys(params_changed)
  };
}

URLUtil.getPath = function(url) {
  return url.split("?")[0]; // FIXME
};

URLUtil.getParams = function(url) {
  var res = {};

  var a = document.createElement('a');
  a.href = url;
  var query_string = a.search.substr(1);
  var params = query_string.split("&");
  for (var i = 0; i < params.length; i++) {
    var p = params[i].split("=");

    var key;
    var value;
    switch (p.length) {
      case 2:
        value = p[1];
        /* fallthrough */
      case 1:
        key = p[0];
        break;
      default:
        continue;
    }

    res[decodeURIComponent(key)] = decodeURIComponent(value);
  }

  return res;
}

URLUtil.getPathAndQuery = function(url) {
  var a = document.createElement('a');
  a.href = url;

  var path = a.pathname;
  if (a.search) {
    path += a.search;
  }

  return path;
}

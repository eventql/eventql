UrlUtil = {};

/**
  * Change an Url's Param Value if key is set or add otherwise
  *
  * @param url the url that should be changed
  * @param m_params list of key, value pairs [{key => key, value => value}, ...]
  */
UrlUtil.addOrModifyUrlParams = function(url, m_params) {
  var url_parts = url.split("?");
  var params = [];
  var parts_splitted = [];

  //query string
  if (url_parts.length >= 2 && url_parts[1].length > 0) {
    params = url_parts[1].split(/[&;]/g);
    parts_splitted = url_parts[1].split(/[&=;]/g);
  }

  for (var i = 0; i < m_params.length; i ++) {
    var key = encodeURIComponent(m_params[i].key);
    var value = encodeURIComponent(m_params[i].value);

    if (parts_splitted.indexOf(key) > - 1) {
      //modify
      for (var j = 0; j < params.length; j++) {

        if (params[j].lastIndexOf(key, 0) > -1) {
          key += "=";
          var param = params[j].split("=");

          if (param.length != 2) {
            return url;
          }

          param[1] = value;
          params[j] = param.join("=");
        }
      }
    } else {
      //add
      params.push(key + "=" + value);
    }
  }

  return url_parts[0] + "?" + params.join('&');
};


UrlUtil.addOrModifyUrlParam = function(url, param, new_value) {
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

//params = [{key: key, value: value}, ...]
UrlUtil.addUrlParams = function(url, params) {
  if (url.indexOf("?") == -1) {
    url += "?";
  } else {
    url += "&";
  }

  for (var i = 0; i < params.length; i++) {
    if (i > 0) {
      url += "&";
    }

    url += params[i].key + "=" + params[i].value;
  }

  return url;
}


UrlUtil.replaceOrAddUrlParam = function(url, param_pattern, new_param) {
  var old_param = url.match(param_pattern);

  if (old_param) {
    url = url.replace(param_pattern, new_param.key + "=" + new_param.value);
  } else {
    url = UrlUtil.addUrlParams(url, [new_param]);
  }

  return url;
};

/**
  * Remove a Param (key=value pair) from an Url
  *
  * @param url the url whose param should be removed
  * @param key the param's key that is removed
  */
UrlUtil.removeUrlParam = function(url, key) {
  var url_parts = url.split("?");

  if (url_parts.length >= 2) {
    var key = encodeURIComponent(key) + "=";
    var params = url_parts[1].split(/[&;]/g);

    for (var i = 0; i < params.length; i++) {
      if (params[i].lastIndexOf(key, 0) > -1) {
        params.splice(i, 1);
      }
    }

    return url_parts[0] + "?" + params.join('&');
  }

  return url;
};


UrlUtil.getParam = function(url, key) {
  var url_parts = url.split("?");

  if (url_parts.length >= 2) {
    var key = encodeURIComponent(key) + "=";
    var params = url_parts[1].split(/[&;]/g);

    for (var i = 0; i < params.length; i++) {
      if (params[i].lastIndexOf(key, 0) > -1) {
        return decodeURIComponent(params[i]);
      }
    }

  }
};

UrlUtil.getParamValue = function(url, key) {
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

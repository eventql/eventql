var API = function(auth_data) {
  'use strict';

  const HOSTNAME = document.location.hostname;
  const PORT = document.location.port;

  var database;

  this.setDatabase = function(db) {
    database = db;
  };

  this.executeSQLSSE = function(query) {
    var url = "/api/v1/sql?format=json_sse&query=" + encodeURIComponent(query);

    if (database) {
      url += "&database=" + encodeURIComponent(database);
    }

    return httpGetSSE(url);
  }

  this.listTables = function(params, callback) {
    httpGet("/api/v1/tables?" + zURLUtil.buildQueryString(params), callback);
  };

  function httpGetSSE(url) {
    var headers = [];
    if (auth_data.api_token) {
      headers.push([ "Authorization", "Token " + auth_data.api_token ]);
    }

    if (auth_data.user && auth_data.password) {
      headers.push([
          "Autorization",
          "Basic " + auth_data.user + ":" + auth_data.password
      ]);
    }

    return new EventSource(url, { "headers": headers });
  }

  function httpGet(url, callback) {
    var headers = {
      "Authorization": "Token " + api_token
    };

    HTTP.httpGet(api_url + url, headers, function(http) {
      var res = {};
      if (http.status >= 200 && http.status < 300) {
        res.status = "success";
        res.success = true;
        res.result = JSON.parse(http.responseText);
      } else {
        res.status = "error";
        res.error = http.responseText;
      }

      callback(res);
    });
  }
};


'use strict'

const http = require('http');
const EventSource = require("eventsource");
const AuthProvider = require('./AuthProvider');
const Query = require('./Query');

class Client {
  constructor (host, port, auth_provider) {
    if (typeof host !== 'string') {
      throw new Error('Please provide host as string')
    }

    if (typeof port !== 'number') {
      throw new Error('Please provide port as a number')
    }

    if (!(auth_provider instanceof AuthProvider)) {
      throw new Error('Please provide an instance of class Auth');
    }

    this.api_path = "/api/v1";
    this.host = host;
    this.port = port;
    this.auth_provider = auth_provider;
  }

  query(query_str) {
    return new Query(this, query_str);
  }

  insert(body, callback) {
    const json = JSON.stringify(body);
    const headers = Object.assign({}, {
      'Content-Type': 'application/json',
      'Accept': 'application/json',
      'Content-Length': Buffer.byteLength(json)
    });

    var req = http.request({
      host: this.host,
      port: this.port,
      path: `${this.api_path}${"/tables/insert"}`,
      method: "POST",
      headers: headers
    }, function(res) {
      var status_code = res.statusCode;
      if (status_code == 201) {
        callback();
        return;
      }

      var body = ""
      res.setEncoding("utf8");
      res.on("data", function(chunk) {
        body += chunk;
      });

      res.on("end", function() {
        callback({code: status_code, message: body});
      });
    });

    req.on("error", function(e) {
      callback({message: e});
    });

    req.write(json);
    req.end();
  }
}

module.exports = Client

"use strict";

const http = require('http');
const EventSource = require("eventsource");

class Query {
  constructor (client, query_str, opts = {}) {
    if (typeof query_str !== 'string') {
      throw new Error('The query must be a string')
    }

    if (typeof opts !== 'object') {
      throw new Error("opts must be an object");
    }

    this.client = client;
    this.query_str = query_str;
    this.opts = opts;
  }

  execute(success_callback, error_callback) {
    let postdata = {
      'query': this.query_str,
      'format': "json"
    }

    if (this.client.auth_provider.hasDatabase()) {
      postdata.database = this.client.auth_provider.database;
    }

    let json = JSON.stringify(postdata);
    const headers = Object.assign({}, {
      'Content-Type': 'application/json',
      'Accept': 'application/json',
      'Content-Length': Buffer.byteLength(json)
    });

    var req = http.request({
      host: this.client.host,
      port: this.client.port,
      path: "/api/v1/sql",
      method: "POST",
      headers: headers
    }, function(res) {
      var status_code = res.statusCode;

      var body = ""
      res.setEncoding("utf8");
      res.on("data", function(chunk) {
        body += chunk;
      });

      res.on("end", function() {
        if (status_code != 200) {
          error_callback({code: status_code, message: body});
        } else {
          success_callback(JSON.parse(body));
        }
      });
    });

    req.on("error", function(e) {
      error_callback({message: e});
    });

    req.write(json);
    req.end();
  }

  executeSSE(callback_options) {
    const params = `format=json_sse&database=${encodeURIComponent(this.client.auth_provider.database)}&query=${encodeURIComponent(this.query_str)}`
    const url = `http://${this.client.host}:${this.client.port}/api/v1/sql?${params}`;
    console.log(url);

    let finished = false;
    const source = new EventSource(url, { headers: {} });
    source.addEventListener("progress", function(e) {
      if (callback_options.onProgress) {
        callback_options.onProgress(e.data);
      }
    });

    source.addEventListener("result", function(e) {
      if (callback_options.onResult) {
        callback_options.onResult(JSON.parse(e.data));
      }
      source.close();
    });

    source.addEventListener("query_error", function(e) {
      if (callback_options.onError) {
        callback_options.onError("query_error", e.data);
      }
      source.close();
    });

    source.addEventListener("error", function(e) {
      if (callback_options.onError) {
        callback_options.onError("error", e.data);
      }
      source.close();
    });

  }
}

module.exports = Query

var Z1 = (function(global) {
  var seq = 0;
  var jobs = {};
  var bcastdata = {};

  function mkJobID() {
    return "job-" + ++seq;
  };

  function executeJob(root_job) {
    var dependencies = [];
    var dependencies_set = {};
    dependencies.push(root_job);

    var find_dependecies = function(job) {
      if (job.sources) {
        job.sources.forEach(function(job_id) {
          if (dependencies_set[job_id]) {
            return;
          }

          var job = jobs[job_id];
          if (!job) {
            throw "invalid job id: " + job_id;
          }

          dependencies_set[job_id] = true;
          dependencies.push(job);
          find_dependecies(job);
        });
      }
    };

    find_dependecies(root_job);
    z1_executemr(JSON.stringify(dependencies), root_job.id);
  }

  /* public api */
  var api = {};

  api.log = function() {
    var parts = [];

    for (var i = 0; i < arguments.length; ++i) {
      parts.push(String(arguments[i]));
    }

    z1_log(parts.join(", "));
  }

  api.broadcast = function() {
    for (var i = 0; i < arguments.length; ++i) {
      var var_name = arguments[i];

      if (typeof var_name != "string")  {
        throw "arguments to Z1.broadcast must be strings";
      }

      if (var_name == "params") {
        throw "'params' is a reserved variable and cannot be broadcasted";
      }

      if (!global.hasOwnProperty(var_name)) {
        throw "no such variable in the global namespace: '" + var_name +
              "' -- all broadcast variables must be global";
      }

      bcastdata[var_name] = String(global[var_name]);
    }
  };

  api.mapTable = function(opts) {
    var job_id = mkJobID();

    jobs[job_id] = {
      id: job_id,
      op: "map_table",
      table_name: opts["table"],
      from: opts["from"],
      until: opts["until"],
      map_fn: String(opts["map_fn"]),
      globals: JSON.stringify(bcastdata),
      params: JSON.stringify(opts["params"] || {})
    };

    return job_id;
  };

  api.reduce = function(opts) {
    var job_id = mkJobID();

    jobs[job_id] = {
      id: job_id,
      op: "reduce",
      sources: opts["sources"],
      num_shards: opts["shards"],
      reduce_fn: String(opts["reduce_fn"]),
      globals: JSON.stringify(bcastdata),
      params: JSON.stringify(opts["params"] || {})
    };

    return job_id;
  };

  api.downloadResults = function(sources) {
    executeJob({
      id: mkJobID(),
      op: "return_results",
      sources: sources
    });
  };

  api.saveToTable = function(opts) {
    executeJob({
      id: mkJobID(),
      op: "save_to_table",
      table_name: opts["table"],
      sources: opts["sources"]
    });
  };

  api.saveToTablePartition = function(opts) {
    executeJob({
      id: mkJobID(),
      op: "save_to_table_partition",
      table_name: opts["table"],
      partition_key: opts["partition"],
      sources: opts["sources"]
    });
  };

  api.processStream = function(opts) {
    var calculate_fn = opts["calculate_fn"];

    var partitions = z1_listpartitions(
        "" + opts["table"],
        "" + opts["from"],
        "" + opts["until"]);

    partitions.forEach(function(partition) {
      var partition_sources = calculate_fn(
          parseInt(partition.time_begin, 10),
          parseInt(partition.time_limit, 10));

      if (typeof partition_sources != "object") {
        throw "Z1.processStream calculate_fn must return a list of jobs";
      }

      api.saveToTablePartition({
        table: opts["table"],
        partition: partition.partition_key,
        sources: partition_sources
      });
    });
  }

  return api;
})(this);

var console = {
  log: function() {
    Z1.log.apply(this, arguments);
  }
};

function __call_with_iter(key, iter) {
  var iter_wrap = (function(iter) {
    return {
      hasNext: function() { return iter.hasNext.apply(iter, arguments); },
      next: function() { return iter.next.apply(iter, arguments); },
    };
  })(iter);

  return __fn.call(this, key, iter_wrap);
}

var __load_closure = (function(global_scope) {
  return function(fn, globals_json, params_json) {
    var globals = JSON.parse(globals_json);
    for (k in globals) {
      eval(k + " = " + globals[k]);
    }

    global_scope["params"] = JSON.parse(params_json);
    eval("__fn = " + fn);
  }
})(this);

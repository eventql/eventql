var console = {
  log: function(str) {
    z1_log(str);
  }
};

var Z1 = (function(global) {
  var seq = 0;
  var jobs = (global["__z1_mr_jobs"] = []);
  var bcastdata = {};
  var cls = {};

  function mkJobID() {
    return "job-" + ++seq;
  };

  cls.broadcast = function(var_name) {
    if (var_name == "params") {
      throw "'params' is a reserved variable and cannot be broadcasted";
    }

    if (!global[var_name]) {
      throw "no such variable: " + var_name;
    }

    bcastdata[var_name] = global[var_name];
  };

  cls.mapTable = function(opts) {
    var job_id = mkJobID();

    jobs.push({
      id: job_id,
      op: "map_table",
      table_name: opts["table"],
      from: opts["from"],
      until: opts["until"],
      map_fn: String(opts["map_fn"]),
      globals: JSON.stringify(bcastdata),
      params: JSON.stringify({ "fu": 123 })
    });

    return job_id;
  };

  cls.reduce = function(opts) {
    var job_id = mkJobID();

    jobs.push({
      id: job_id,
      op: "reduce",
      sources: opts["sources"],
      num_shards: opts["shards"],
      reduce_fn: String(opts["reduce_fn"]),
      globals: JSON.stringify(bcastdata),
      params: JSON.stringify({ "fu": 123 })
    });

    return job_id;
  };

  cls.downloadResults = function(sources) {
    var job_id = mkJobID();

    jobs.push({
      id: job_id,
      op: "return_results",
      sources: sources
    });

    return job_id;
  };

  cls.saveToTable = function(opts) {
    var job_id = mkJobID();

    jobs.push({
      id: job_id,
      op: "save_to_table",
      table_name: opts["table"],
      sources: opts["sources"]
    });

    return job_id;
  };

  cls.saveToTablePartition = function(opts) {
    var job_id = mkJobID();

    jobs.push({
      id: job_id,
      op: "save_to_table_partition",
      table_name: opts["table"],
      partition_key: opts["partition"],
      sources: opts["sources"]
    });

    return job_id;
  };

  cls.processStream = function(opts) {
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

      mkSaveToTablePartitionTask({
        table: opts["table"],
        partition: partition.partition_key,
        sources: partition_sources
      });
    });
  }

  return cls;
})(this);

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
      global_scope[k] = globals[k];
    }

    global_scope["params"] = JSON.parse(params);
    eval("__fn = " + fn);
  }
})(this);

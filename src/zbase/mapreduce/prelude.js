var console = {
  log: function(str) {
    z1_log(str);
  }
};

var Z1 = (function(global) {
  var seq = 0;
  var jobs = (global["__z1_mr_jobs"] = []);
  var bcastdata = {};

  var mkMapTableTask = function(opts) {
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

  var mkReduceTask = function(opts) {
    var job_id = mkJobID();

    var reduce_fn_id = mkFnID();
    global[reduce_fn_id] = opts["reduce_fn"];

    jobs.push({
      id: job_id,
      op: "reduce",
      sources: opts["sources"],
      num_shards: opts["shards"],
      method_name: reduce_fn_id
    });

    return job_id;
  };

  var mkDownloadResultsTask = function(sources) {
    var job_id = mkJobID();

    jobs.push({
      id: job_id,
      op: "return_results",
      sources: sources
    });

    return job_id;
  };

  var mkSaveToTableTask = function(opts) {
    var job_id = mkJobID();

    jobs.push({
      id: job_id,
      op: "save_to_table",
      table_name: opts["table"],
      sources: opts["sources"]
    });

    return job_id;
  };

  var mkSaveToTablePartitionTask = function(opts) {
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

  var processStream = function(opts) {
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

  var mkJobID = function() {
    return "job-" + ++seq;
  };

  var mkFnID = function() {
    return "__z1_mr_fn_" + ++seq;
  };

  var cls = {
    mapTable: mkMapTableTask,
    reduce: mkReduceTask,
    downloadResults: mkDownloadResultsTask,
    saveToTable: mkSaveToTableTask,
    saveToTablePartition: mkSaveToTablePartitionTask,
    processStream: processStream
  };

  cls.broadcast = function(var_name) {
    if (!global[var_name]) {
      throw "no such variable: " + var_name;
    }

    bcastdata[var_name] = global[var_name];
  };

  return cls;
})(this);

function __call_with_iter(method, key, iter) {
  var iter_wrap = (function(iter) {
    return {
      hasNext: function() { return iter.hasNext.apply(iter, arguments); },
      next: function() { return iter.next.apply(iter, arguments); },
    };
  })(iter);

  return this[method].call(this, key, iter_wrap);
}

var __load_closure = (function(global_scope) {
  return function(fn, globals_json, params_json) {
    var globals = JSON.parse(globals_json);
    for (k in globals) {
      global_scope[k] = globals[k];
    }

    eval("__fn = " + fn);
  }
})(this);

var EventSourceHandler = function() {
  var sources = {};


  function get(id, url) {
    // set sources entry
    if (!sources.hasOwnProperty[id]) {
      sources[id] = {
        running: false,
        source: null
      };
    }

    // if source with same id is already running close this source
    if (sources[id].running) {
      sources[id].source.close();
      sources[id].source = null;
    }

    sources[id].running = true;
    sources[id].source = new EventSource(url);

    return sources[id].source;
  };

  function close(id) {
    if (sources[id] && sources[id].running) {
      sources[id].source.close();
      sources[id].running = false;
    }
  };

  function closeAll() {
    for (var id in sources) {
      close(id);
    }
  }

  return {
    get: get,
    close: close,
    closeAll: closeAll
  }
};

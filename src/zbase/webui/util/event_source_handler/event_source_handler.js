var EventSourceHandler = function() {
  var sources = {};

  function get(id, url) {
    close(id);
    sources[id] = new EventSource(url);
    return sources[id];
  };

  function close(id) {
    if (sources[id]) {
      sources[id].close();
      delete sources[id];
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

EventSourceHandler = function() {
  var sources = {};

  function get(id, url) {
    if (!sources.hasOwnProperty[id]) {
      sources[id] = {
        running: false,
        source: null
      };
    }

    //close still running source
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

  function onUnload() {
    for (var id in sources) {
      close(id);
    }
    window.removeEventListener('beforeunload', onUnload);
  }

  window.addEventListener('beforeunload', onUnload);

  return {
    get: get,
    close: close
  }
};

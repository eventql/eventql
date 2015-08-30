var HeaderWidget = (function() {

  var render = function() {
    var conf = $.getConfig();
    var tpl = $.getTemplate("widgets/zbase-header", "zbase_header_tpl");

    var elem = document.querySelector("#zbase_header");
    elem.innerHTML = "";
    elem.appendChild(tpl)
    elem.querySelector(".userid_info").innerHTML = conf.current_user.userid;
    elem.querySelector(".namespace_info").innerHTML = conf.current_user.namespace;

    setActiveMenuItem();
    elem.classList.remove("hidden");

    $.handleLinks(elem);
  };

  var setActiveMenuItem = function() {
    var path = window.location.pathname;
    var elem = $("#zbase_header");
    var items = elem.querySelectorAll(".nav");
    var active_item;

    for (var i = 0; i < items.length; i++) {
      if (path.indexOf(items[i].getAttribute("href")) == 0) {
        active_item = items[i];
      }
    }

    active_item.classList.add("active");
  };

  return {
    render: render
  };

})();

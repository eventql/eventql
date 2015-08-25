var HeaderWidget = (function() {

  var render = function() {
    var conf = $.getConfig();
    var tpl = $.getTemplate("widgets/zbase-header", "zbase_header_tpl");

    var elem = document.querySelector("#zbase_header");
    elem.innerHTML = "";
    elem.appendChild(tpl)
    elem.querySelector(".userid_info").innerHTML = conf.current_user.userid;
    elem.querySelector(".namespace_info").innerHTML = conf.current_user.namespace;
    elem.classList.remove("hidden");

    $.handleLinks(elem);
  };

  return {
    render: render
  };

})();

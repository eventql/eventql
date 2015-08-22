ZBase.util.header_widget = (function() {

  var render = function() {
    var conf = ZBase.getConfig();
    var elem = document.querySelector("#zbase_header");

    elem.innerHTML = "";
    elem.appendChild(ZBase.getTemplate("header_widget", "zbase_header_tpl"))
    elem.querySelector(".userid_info").innerHTML = conf.current_user.userid;
    elem.querySelector(".namespace_info").innerHTML = conf.current_user.namespace;
    elem.classList.remove("hidden");

    ZBase.util.install_link_handlers(elem);
  };

  return {
    render: render
  };

})();

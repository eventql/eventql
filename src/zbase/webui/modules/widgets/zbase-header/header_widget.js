var HeaderWidget = (function() {

  var render = function() {
    var conf = $.getConfig();
    var elem = document.querySelector("#zbase_header");

    elem.innerHTML = "";
    elem.appendChild($.getTemplate("header_widget", "zbase_header_tpl"))
    elem.querySelector(".userid_info").innerHTML = conf.current_user.userid;
    elem.querySelector(".namespace_info").innerHTML = conf.current_user.namespace;
    elem.classList.remove("hidden");

    $.handleLinks(elem);
  };

  return {
    render: render
  };

})();

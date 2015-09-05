var HeaderWidget = (function() {

  var toggleDropdown = function() {
    this.classList.toggle("open");
  }

  var popup = function() {
    var tpl = $.getTemplate("widgets/zbase-header", "zbase_header_namespace_prompt_tpl");
    var elem = document.body.appendChild(document.createElement("div"));
    elem.setAttribute("id", "namespace_popup");
    elem.appendChild(tpl);
    elem.querySelector(".go_back").addEventListener("click", function(e) {
      e.preventDefault();
      document.body.removeChild(elem);
    });

    var list = elem.querySelector(".namespace_list");
    list.innerHTML = "";

    $.httpGet("/api/v1/auth/available_namespaces", function(res) {
      // Replace with parsing of actual response.
      res = ["namespace1", "namespace2", "namespace3"];

      res.forEach(function(namespace) {
        var namespaceElem = document.createElement("li");
        namespaceElem.setAttribute("data-namespace", namespace);
        namespaceElem.innerHTML = namespace;
        namespaceElem.addEventListener("click", (function(namespace) {
          return function() {
            // Replace with proper format for sending to API and any additonal needed function calls.
            var postdata = $.buildQueryString({namespace: namespace});

            $.httpPost("/api/v/1/auth/set_namespace", postdata, function(res) {
              console.log("Sent namespace change request: " + namespace);
            })
          }
        })(namespace));
        list.appendChild(namespaceElem);
      });
    });
  }

  var render = function() {
    var conf = $.getConfig();
    var tpl = $.getTemplate("widgets/zbase-header", "zbase_header_tpl");

    var elem = document.querySelector("#zbase_header");
    elem.innerHTML = "";
    elem.appendChild(tpl);
    elem.querySelector(".dropdown").addEventListener("click", toggleDropdown);
    elem.querySelector("#change_namespace").addEventListener("click", popup);
    elem.querySelector(".userid_info").innerHTML = conf.current_user.userid;
    elem.querySelector(".namespace_info").innerHTML = conf.current_user.namespace;
    elem.classList.remove("hidden");

    $.handleLinks(elem);
  };

  return {
    render: render
  };

})();

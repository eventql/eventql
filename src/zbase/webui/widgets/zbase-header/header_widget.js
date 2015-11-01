var HeaderWidget = (function() {

  var toggleDropdown = function() {
    this.classList.toggle("open");
  }

  var showSelectNamespacePopup = function() {
    ZBase.showLoader();
    ZBase.loadModules(["widgets/z-modal"], function() {
      var tpl = $.getTemplate(
          "widgets/zbase-header",
          "zbase_header_namespace_prompt_tpl");
      var modal = $("z-modal", tpl);
      document.body.appendChild(modal);

      modal.addEventListener("z-modal-close", function() {
        document.body.removeChild(modal);
      });

      var list = modal.querySelector(".namespace_list");
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
          ZBase.hideLoader();
          modal.show();
        });
      });
    });
  }

  var render = function() {
    var conf = $.getConfig();
    var tpl = $.getTemplate("widgets/zbase-header", "zbase_header_tpl");

    $(".userid_info", tpl).innerHTML = conf.current_user.userid;
    $(".namespace_info", tpl).innerHTML = conf.current_user.namespace;
    $(".change_namespace", tpl).addEventListener("click", showSelectNamespacePopup);
    $.onClick($(".dropdown", tpl), toggleDropdown);
    $("z-search", tpl).addEventListener("z-search-autocomplete", searchAutocomplete);

    var elem = $("#zbase_header");
    $.replaceContent(elem, tpl);
    elem.classList.remove("hidden");

    document.addEventListener("click", function(event) {
      if ((function getParentWithClass(el, className) {
        while ((el = el.parentElement) && !el.classList.contains(className));
        return el;
      })(event.target, "dropdown")) return;

      $(".dropdown", elem).classList.remove("open");
    });

    $.handleLinks(elem);
  };

  var searchAutocomplete = function(e) {
    var term = e.detail.value;
    var search_widget = this;

    $.httpGet("/api/v1/documents?search=" + term, function(r) {
      if (r.status == 200) {
        var documents = JSON.parse(r.response).documents;
        var items = [];

        for (var i = 0; i < documents.length; i++) {
          if (i > 10) {
            break;
          }

          items.push({
            query: documents[i].name,
            data_value: documents[i].name});
        }

        search_widget.autocomplete(term, items);
      }
    });
  };

  var setActiveMenuItem = function() {
    var path = window.location.pathname;
    var elem = $("#zbase_header");
    var items = elem.querySelectorAll(".nav");
    var prev_active_item = $(".nav.active", elem);
    var active_item;

    if (prev_active_item) {
      prev_active_item.classList.remove("active");
    }

    for (var i = 0; i < items.length; i++) {
      if (path.indexOf(items[i].getAttribute("href")) == 0) {
        active_item = items[i];
      }
    }

    if (active_item) {
      active_item.classList.add("active");
    }
  };

  return {
    render: render,
    setActiveItem: setActiveMenuItem
  };

})();

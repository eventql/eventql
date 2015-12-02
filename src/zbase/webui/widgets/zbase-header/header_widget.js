var HeaderWidget = (function() {

  var render = function() {
    var conf = $.getConfig();

    /* user navi */
    var usernav_tpl = $.getTemplate(
        "widgets/zbase-header",
        "zbase_header_user_navi_tpl");

    $(".userid_info", usernav_tpl).innerHTML = conf.current_user.userid;
    $(".namespace_info", usernav_tpl).innerHTML = conf.current_user.namespace;
    $.onClick($(".dropdown", usernav_tpl), toggleDropdown);
    $(".change_namespace", usernav_tpl).addEventListener(
        "click",
        showSelectNamespacePopup);
    $.handleLinks(usernav_tpl);
    $.replaceContent($("#zscale_user_navi"), usernav_tpl);

    //document.addEventListener("click", function(event) {
    //  if ((function getParentWithClass(el, className) {
    //    while ((el = el.parentElement) && !el.classList.contains(className));
    //    return el;
    //  })(event.target, "dropdown")) return;

    //  $(".dropdown", elem).classList.remove("open");
    //});

    /* main manivi */
    var mainnav_tpl = $.getTemplate("widgets/zbase-header", "zbase_header_main_navi_tpl");
    $.handleLinks(mainnav_tpl);
    $.replaceContent($("#zscale_navi .zscale_navi_menu"), mainnav_tpl);

    /* header */
    var hdr_tpl = $.getTemplate("widgets/zbase-header", "zbase_header_tpl");
    $("z-dropdown.new_query", hdr_tpl).addEventListener("change", createNewDocument);
    $.handleLinks(usernav_tpl);
    $.replaceContent($("#zbase_header"), hdr_tpl);
  };

  var update = function(path) {};

  var setBreadCrumbs = function(crumbs) {
    var breadcrumbs = $(".header_bar zbase-breadcrumbs");
    var inner = document.createElement("div");

    crumbs.forEach(function(crumb) {
      var section = document.createElement("zbase-breadcrumbs-section");

      if (crumb.href) {
        var link = document.createElement("a");
        link.innerHTML = crumb.title;
        link.href = crumb.href;
        section.appendChild(link);
      }

      inner.appendChild(section);
    });

    $.replaceContent(breadcrumbs, inner);
  };

  var toggleDropdown = function() {
    this.classList.toggle("open");
  };

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
  };

  var createNewDocument = function() {
    var doc_type = this.getValue();
    this.setValue([]);
    $.createNewDocument(doc_type);
  };

  return {
    render: render,
    update: update,
    setBreadCrumbs: setBreadCrumbs
  };

})();

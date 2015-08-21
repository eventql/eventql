ZBase.registerView((function() {

  var submitForm = function(e) {
    e.preventDefault();

    var postdata = ZBase.util.buildQueryString({
      userid: this.querySelector("input[name='userid']").value,
      password: this.querySelector("input[name='password']").value
    });

    console.log(postdata);
    ZBase.util.httpPost("/analytics/api/v1/auth/login", postdata, function(http) {
      if (http.status == 200) {
        ZBase.navigateTo("/a/");
        return;
      }

      if (http.status == 401) {
        showErrorMessage("Invalid Credentials");
        return;
      }

      showErrorMessage("Server Error; please try again and contact support if the problem persists.");
    });

    return false;
  };

  var hideErrorMessage = function() {
    var viewport = document.getElementById("zbase_viewport");
    var elem = viewport.querySelector(".zbase_login .error_message");
    elem.classList.add("hidden");
  };

  var showErrorMessage = function(msg) {
    var viewport = document.getElementById("zbase_viewport");
    var elem = viewport.querySelector(".zbase_login .error_message");
    elem.innerHTML = msg;
    elem.classList.remove("hidden");
  };

  var render = function(path) {
    var viewport = document.getElementById("zbase_viewport");
    var page = ZBase.getTemplate("login", "zbase_login_form_tpl");

    var form = page.querySelector("form");
    form.addEventListener("submit", submitForm);

    viewport.innerHTML = "";
    viewport.appendChild(page);
  };

  return {
    name: "login",
    loadView: function(params) { render(params.path); },
    unloadView: function() {},
    handleNavigationChange: render
  };
})());

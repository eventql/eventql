ZBase.registerView((function() {

  var submitForm = function(e) {
    e.preventDefault();

    var username = this.querySelector("input[name='userid']").value;
    var password = this.querySelector("input[name='password']").value;

    console.log("login", username, password);

    ZBase.util.httpPost("/api/v1/auth/login", "", function(http) {
      //if (http.status == 200) {
      showErrorMessage("Invalid Credentials");
    });

    return false;
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

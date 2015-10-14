var ZbaseInfoMessage = function(elem) {
  var renderSuccess = function(message) {
    $(".inner", tpl).innerHTML = message;
    display();
    window.setTimeout(hide, 3000);
  };

  var renderError = function(message) {
    var inner = $(".inner", tpl);
    inner.classList.add("error");
    inner.innerHTML = message;
    display();

    window.setTimeout(function() {
      hide();
      inner.classList.remove("error");
    }, 4000);
  };

  var display = function() {
    tpl.classList.remove("hidden");
  };

  var hide = function() {
    tpl.classList.add("hidden");
  };

  var tpl = $(
      ".zbase_info_message",
      $.getTemplate(
        "widgets/zbase-info-message",
        "zbase_info_message_tpl"));

  elem.appendChild(tpl);

  return {
    renderSuccess: renderSuccess,
    renderError: renderError
  }
};

var ZbaseInfoMessage = function(elem) {
  var render = function(message) {
    $(".inner", tpl).innerHTML = message;
    tpl.classList.remove("hidden");

    window.setTimeout(hide, 3000);
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
    render: render
  }
};

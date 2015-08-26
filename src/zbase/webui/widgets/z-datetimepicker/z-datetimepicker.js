/**
  * require_module: "z-calendar"
  * require_module: "z-button"
 **/

var DateTimePicker = function(input) {
  var tpl = $.getTemplate("widgets/z-datetimepicker", "z-datetimepicker-base-tpl");
  var proto;
  if (input.tagName == "Z-INPUT") {
    input = input.querySelector("input");
  }

  this.hide = function() {
    proto.removeAttribute("data-active");
  };

  this.show = function() {
    proto.setAttribute("data-active", "active");
  };

  /*************************** PRIVATE **********************************/
  this.__init = function() {
    // insert tpl after input
    input.parentNode.insertBefore(tpl, input.nextSibling);
    //FIXME
    proto = input.nextElementSibling;
    var _this = this;
    proto.querySelector("button").addEventListener("click", function() {
      _this.__apply();
    }, false);

    this.__onTimeChange();
    this.__handleVisibility();
    this.__controlTimeInput();
  };

  this.__handleVisibility = function() {
    var _this = this;

    input.addEventListener("click", function() {
      if (proto.hasAttribute("data-active")) {
        _this.hide();
      } else {
        _this.show();
      }
    }, false);
  };

  this.__controlTimeInput = function() {
    var inputs = proto.querySelectorAll("input");
    var _this = this;

    for (var i = 0; i < inputs.length; i++) {
      inputs[i].addEventListener("blur", function() {

        switch (this.value.length) {
          // if no value is given set it to 00
          case 0:
            this.value = "00";
            break;

          case 1:
            var value = parseInt(this.value, 10);
            // non-integer value
            if (isNaN(value)) {
              _this.__renderTimeControlError(this);
              return;
            }
            this.value = Fnord.appendLeadingZero(value);
            break;

          case 2:
            // non-integer value
            if (isNaN(parseInt(this.value[0], 10)) ||
                isNaN(parseInt(this.value[1], 10))) {
              _this.__renderTimeControlError(this);
              return;
            }
            break;
        }

        var value = parseInt(this.value, 10);
        if (this.getAttribute("data-factor") == "3600") {
          // hours value > 24
          if (value > 23) {
            _this.__renderTimeControlError(this);
            return;
          }
        } else {
          // minutes or seconds value > 59
          if (value > 59) {
            _this.__renderTimeControlError(this);
            return;
          }
        }


        // correct input value
        _this.__removeTimeControlError(this);
      }, false);
    }
  };

  this.__removeTimeControlError = function(time_control) {
    time_control.classList.remove("error");
    // if only correct input values enable apply button
    if (proto.querySelector("input.error") == null) {
      proto.querySelector("button").removeAttribute("data-state");
    }
  };

  this.__renderTimeControlError = function(time_control) {
    time_control.classList.add("error");
    // disable apply button
    proto.querySelector("button").setAttribute("data-state", "disabled");
  };

  this.__onTimeChange = function() {
    var timestamp = Math.floor(
      parseInt(input.getAttribute("data-timestamp"), 10));

    // set input value
    input.value = DateUtil.printTimestamp(timestamp);

    timestamp = Math.floor(timestamp / 1000);
    var date = new Date(timestamp);

    // set z-calendar selection
    proto.querySelector("z-calendar").setAttribute("data-selected", timestamp);
    // set hours input value
    proto.querySelector("input[data-value='hours']").value =
      DateUtil.appendLeadingZero(date.getHours());
    // set minutes input value
    proto.querySelector("input[data-value='minutes']").value =
      DateUtil.appendLeadingZero(date.getMinutes());
    // set seconds input value
    proto.querySelector("input[data-value='seconds']").value =
      DateUtil.appendLeadingZero(date.getSeconds());
  };

  this.__apply = function() {
    

    // fire change event on input
    var evt = new CustomEvent("z-datetimepicker-change");
    input.dispatchEvent(evt);
    console.log("apply");
  };


  // init
  this.__init();
};

/*var proto = Object.create(HTMLElement.prototype);
DateTimePicker.apply(proto);
document.registerElement("z-datetimepicker", { prototype: proto });*/
/*

    this.handleTimeControl = function() {
      var _this = this;
      var control_input = document.getElementById("time_control");
      var widget = document.getElementById("logviewer_date_widget");
      var inputs = widget.querySelectorAll("input");
      var apply_button = document.querySelector(
        "fn-button[data-action='set-logviewer-date']");

      control_input.addEventListener("click", function() {
        if (widget.classList.contains("hidden")) {
          widget.classList.remove("hidden");
        } else {
          widget.classList.add("hidden")
        }
      }, false);


      for (var i = 0; i < inputs.length; i++) {
        inputs[i].addEventListener("blur", function() {
          switch (this.value.length) {
            case 0:
              this.value = "00";
              break;

            case 1:
              var value = parseInt(this.value, 10);
              if (isNaN(value)) {
                this.classList.add("error");
                apply_button.setAttribute("data-state", "disabled");
                return;
              }
              this.value = Fnord.appendLeadingZero(value);
              break;

            case 2:
              if (isNaN(parseInt(this.value[0], 10)) ||
                  isNaN(parseInt(this.value[1], 10))) {
                this.classList.add("error");
                apply_button.setAttribute("data-state", "disabled");
                return;
              }
          }

          var value = parseInt(this.value, 10);
          if (this.getAttribute("data-factor") == "3600") {
            if (value > 23) {
              this.classList.add("error");
              apply_button.setAttribute("data-state", "disabled");
              return;
            }
          } else {
            if (value > 59) {
              this.classList.add("error");
              apply_button.setAttribute("data-state", "disabled");
              return;
            }
          }

          this.classList.remove("error");
          if (widget.querySelector("input.error") == null) {
            apply_button.removeAttribute("data-state");
          }
        }, false);
      }

      apply_button.addEventListener("click", function() {
        var timestamp = DateUtil.parseTimestamp(
          parseInt(widget.querySelector("fn-calendar")
            .getAttribute("data-selected"), 10));
        for (var i = 0; i < inputs.length; i++) {
          var value = parseInt(inputs[i].value, 10);
          timestamp += value * parseInt(inputs[i].getAttribute("data-factor"), 10);
        }

        timestamp = timestamp * 1000;
        _this.initial_end_time = timestamp;
        _this.updateParams();
        widget.classList.add("hidden");
        history.pushState(
          {param: "until", value: timestamp},
          "Until",
          UrlUtil.addOrModifyUrlParam(
            window.location.href, "until", timestamp));
      }, false);
    };

  */

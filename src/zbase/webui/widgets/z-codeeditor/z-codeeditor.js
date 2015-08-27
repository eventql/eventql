var CodeEditorComponent = function() {

  var initCodeMirror = function(textarea) {
    var codemirror_opts = {
      autofocus: true,
      lineNumbers: true,
      lineWrapping: true
    };

    if (this.hasAttribute("data-readonly")) {
      codemirror_opts.readOnly = true;
      codemirror_opts.autofocus = false;
    }

    var codemirror = CodeMirror.fromTextArea(textarea, codemirror_opts);
    codemirror.setOption("mode", this.getAttribute("data-language"));

    // FIXME horrible horrible hack to work around a bug in codemirror where
    // the editor can't be rendered properly before the browser has actually
    // rendered the backing textarea. yield to the browser and poll for
    // completed rendering
    var poll = (function(base){
      return function() {
        if ($(".CodeMirror-gutter", base).offsetWidth == 0) {
          codemirror.refresh();
          window.setTimeout(poll, 1);
        }
      };
    })(this);
    poll();

    return codemirror;
  }


  var setupKeyPressHandlers = function() {
    var base = this;

    var cmd_pressed = false;
    this.addEventListener('keydown', function(e) {
      if (e.keyCode == 17 || e.keyCode == 91) {
        cmd_pressed = true;
      }
    }, false);

    this.addEventListener('keyup', function(e) {
      if (e.keyCode == 17 || e.keyCode == 91) {
        cmd_pressed = false;
      }
    }, false);

    this.addEventListener('keydown', function(e) {
      if (e.keyCode == 13 && cmd_pressed) {
        e.preventDefault();
        base.execute.call(base);
      }
    }, false);
  };

  this.createdCallback = function() {
    var tpl = $.getTemplate("widgets/z-codeeditor", "z-codeeditor-base-tpl");

    var textarea = document.createElement("textarea");
    //textarea.setAttribute("autofocus", "autofocus");
    this.appendChild(textarea);

    var codemirror = initCodeMirror.call(this, textarea);
    setupKeyPressHandlers.call(this);

    this.getValue = function() {
      return codemirror.getValue();
    };

    this.setValue = function(value) {
      if (!value) {return;}
      codemirror.setValue(value);
    }

    this.execute = function() {
      var ev = new Event('execute');
      ev.value = this.getValue();
      this.dispatchEvent(ev);
    }

    this.setReadonly = function() {
      codemirror.options.readOnly= true;
      codemirror.options.autofocus = false;
    };
  };

  this.attributeChangedCallback = function(attr) {
    switch (attr) {
      case "data-readonly":
        this.setReadonly();
      break;

      default:
        break;
    }
  };
};

var proto = Object.create(HTMLElement.prototype);
CodeEditorComponent.apply(proto);
document.registerElement("z-codeeditor", { prototype: proto });

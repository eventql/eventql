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

  var setupResizer = function() {
    var resizer = this.querySelector("z-codeeditor-resizer-tooltip");
    var gutters = this.querySelector(".CodeMirror-gutters");
    var codemirror = this.querySelector(".CodeMirror");
    var benchmark_y;
    codemirror.style.height = (this.offsetHeight - resizer.offsetHeight) + "px";

    //TODO handle horizontal resizing
    resizer.addEventListener('dragstart', function(e) {
      this.style.background = "transparent";
      this.style.border = "none";
      benchmark_y = e.clientY;
    }, false);

    resizer.addEventListener('drag', (function(elem) {
      return function(e) {
        e.preventDefault();
        this.style.background = "";
        this.style.border = "";
        var offset = benchmark_y - e.clientY;
        var height = elem.offsetHeight - offset;
        elem.style.height = height + "px";
        codemirror.style.height = (height - resizer.offsetHeight) + "px";
        gutters.style.height = height + "px";
        benchmark_y = e.clientY;
      }
    })(this));
  };

  this.createdCallback = function() {
    var tpl = $.getTemplate("widgets/z-codeeditor", "z-codeeditor-base-tpl");
    this.appendChild(tpl);

    var codemirror = initCodeMirror.call(this, this.querySelector("textarea"));
    setupKeyPressHandlers.call(this);

    if (this.hasAttribute("data-resizable")) {
      window.setTimeout(setupResizer.bind(this));
    }

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
        // FIXME: handle the case where the readonly attribute was removed
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

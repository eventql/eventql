var CodeEditorComponent = function() {
  this.createdCallback = function() {
    var shadow = this.createShadowRoot();
    var tpl = document.getTemplateByID("fn-codeeditor-base-tpl");
    shadow.appendChild(tpl);
    var textarea = this.shadowRoot.querySelector("textarea");

    var codemirror = CodeMirror.fromTextArea(
        textarea,
        {
          autofocus: false,
          lineNumbers: true,
          lineWrapping: true
        }
    );

    // FIXME horrible horrible hack to work around a bug in codemirror where
    // the editor can't be rendered properly before the browser has actually
    // rendered the backing textarea. yield to the browser and poll for
    // completed rendering
    var poll = (function(base){
      return function() {
        if (base.shadowRoot.querySelector(".CodeMirror-gutter").offsetWidth == 0) {
          codemirror.refresh();
          window.setTimeout(poll, 1);
        }
      };
    })(this);
    poll();

    this.getValue = function() {
      return codemirror.getValue();
    };

    this.setValue = function(value) {
      if (!value) {return;}
      codemirror.setValue(value);
    }
  };
};

var proto = Object.create(HTMLElement.prototype);
CodeEditorComponent.apply(proto);
document.registerElement("fn-codeeditor", { prototype: proto });

/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
EventQL.CodeEditor = function(elem, params) {
  'use strict';

  var codemirror;
  var execute_callback;

  this.setExecuteCallback = function(callback_fn) {
    execute_callback = callback_fn;
  }

  this.render = function() {
    var tpl = TemplateUtil.getTemplate("evql-codeeditor-tpl");
    elem.appendChild(tpl);

    codemirror = initCodeMirror.call(this, elem.querySelector("textarea"));
    setupKeyPressHandlers.call(this);

    if (params.resizable) {
      window.setTimeout(setupResizer.bind(this));
    }
  }

  this.getValue = function() {
    return codemirror.getValue();
  };

  this.setValue = function(value) {
    if (!value) {return;}
    codemirror.setValue(value);
  }

  this.setReadonly = function(readonly) {
    codemirror.options.readOnly= readonly;
    codemirror.options.autofocus = !readonly;
  };

  this.focus = function() {
    codemirror.focus();
  };

  var initCodeMirror = function(textarea) {
    var codemirror_opts = {
      autofocus: true,
      lineNumbers: true,
      lineWrapping: true
    };

    if (params.readonly) {
      codemirror_opts.readOnly = true;
      codemirror_opts.autofocus = false;
    }

    var codemirror = CodeMirror.fromTextArea(textarea, codemirror_opts);
    codemirror.setOption("mode", params.language);

    // FIXME horrible horrible hack to work around a bug in codemirror where
    // the editor can't be rendered properly before the browser has actually
    // rendered the backing textarea. yield to the browser and poll for
    // completed rendering
    var poll = (function(){
      return function() {
        if (elem.querySelector(".CodeMirror-gutter").offsetWidth == 0) {
          codemirror.refresh();
          window.setTimeout(poll, 1);
        }
      };
    })();
    poll();

    return codemirror;
  }

  function setupKeyPressHandlers() {
    elem.addEventListener('keydown', function(e) {
      //metaKey is cmd key on mac and windows key on windows
      if (e.keyCode == 13 && (e.ctrlKey || e.metaKey)) {
        e.preventDefault();
        if (execute_callback) {
          execute_callback(codemirror.getValue());
        }
      }
    }, false);
  };

  function setupResizer() {
    var resizer = elem.querySelector(".resizer_tooltip")
    var gutters = elem.querySelector(".CodeMirror-gutters");
    var codemirror = elem.querySelector(".CodeMirror");
    var benchmark_y;

    //TODO handle horizontal resizing
    resizer.addEventListener('dragstart', function(e) {
      this.style.background = "transparent";
      this.style.border = "none";
      benchmark_y = e.clientY;
    }, false);

    resizer.addEventListener('drag', (function(editor_elem) {
      return function(e) {
        e.preventDefault();
        this.style.background = "";
        this.style.border = "";
        var offset = benchmark_y - e.clientY;
        var height = elem.offsetHeight - offset;
        editor_elem.style.height = height + "px";
        codemirror.style.height = height + "px";
        gutters.style.height = height + "px";
        benchmark_y = e.clientY;
      }
    })(elem));
  };
};

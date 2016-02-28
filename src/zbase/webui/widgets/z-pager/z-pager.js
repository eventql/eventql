var PagerComponent = function() {
  this.createdCallback = function() {
    var tpl = Fnord.getTemplate("z-pager", "base");
    this.appendChild(tpl);

    if (this.hasAttribute('data-for')) {
      //set classname
    } else {
      this.init();
      this.renderTitle();
      this.observeTooltips(this.pageEvent, this);
    }
  };

  this.attributeChangedCallback = function() {
    this.initLastItem();
    this.initNumPerPage();
    this.initNumPages();
  }

  this.forElement = function(elem) {
    var callback = elem.paginationEvent;

    this.init();
    this.renderTitle();
    this.observeTooltips(callback, elem);
  };

  this.disable = function(tooltip) {
    var _this = this;
    tooltip.forEach(function(t) {
      _this.querySelector(".tooltip." + t).classList.add("disabled");
    });
  };


  this.enable = function(tooltip) {
    var _this = this;
    tooltip.forEach(function(t) {
      _this.querySelector(".tooltip." + t).classList.remove("disabled");
    });
  };


  this.init = function() {
    this.initCurrentPage();
    this.initLastItem();
    this.initNumPerPage();
    this.initNumPages();
  };

  this.initCurrentPage = function() {
    var curr_page = parseInt(this.getAttribute('data-current-page'), 10);

    if (isNaN(curr_page)) {
      curr_page = 0;
    }

    this.current_page = curr_page;
  };

  this.initLastItem = function() {
    var end = parseInt(this.getAttribute('data-last-item'), 10);

    if (isNaN(end)) {
      end = 0;
    }

    this.end = end;
  };

  this.initNumPerPage = function() {
    var per_page = parseInt(this.getAttribute('data-per-page'), 10);

    if (isNaN(per_page)) {
      per_page = 1;
    }

    this.per_page = per_page;
  };

  this.initNumPages = function() {
    this.pages = Math.ceil((this.end - 1) / this.per_page);
  };


  this.renderTitle = function() {
    var current_page = this.current_page;
    var per_page = this.per_page;
    var end = this.end;
    var title = this.querySelector(".title");

    var start = (current_page) * per_page + 1;
    var current_end = start + per_page - 1;

    if (current_end > end) {
      current_end = end;
    }


    title.innerHTML =
      "<b>" + start + "</b> - <b>" + current_end;

    if (this.getAttribute('data-title') == 'semi') {
      return;
    }

    title.innerHTML +=  "</b> of <b>" + end + "</b>";
  };


  this.observeTooltips = function(cb, caller) {
    var backward_tooltip = this.querySelector(".backward");
    var forward_tooltip = this.querySelector(".forward");
    var base = this;

    backward_tooltip.onclick = function() {
      if (this.classList.contains('disabled')) {
        return;
      }

      var pages = base.pages;
      var current_page = base.current_page;

      if (current_page > 0 || base.hasAttribute('data-circling')) {
        var new_page = (current_page + pages - 1) % pages;
        cb(new_page, caller);
        base.current_page = new_page;
        base.renderTitle();
      }
    };

    forward_tooltip.onclick = function() {
      if (this.classList.contains('disabled')) {
        return;
      }

      var pages = base.pages;
      var current_page = base.current_page;

      if (current_page < pages - 1) {
        base.current_page =  current_page + 1;
        base.renderTitle();
        cb(current_page + 1, caller);
      } else {
        if (base.hasAttribute('data-circling')) {
          base.current_page = 0;
          base.renderTitle();
          cb(0, caller);
        }
      }
    };
  };

  this.pageEvent = function(current_page, base) {
    var page_event = new CustomEvent("z-pager-turn", {detail : current_page});
    base.dispatchEvent(page_event);
  };

};

var proto = Object.create(HTMLElement.prototype);
PagerComponent.apply(proto);
document.registerElement("z-pager", { prototype: proto });

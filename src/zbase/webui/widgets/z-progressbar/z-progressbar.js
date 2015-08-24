var ProgressBarComponent = function() {
  this.createdCallback = function() {
    var tpl = $.getTemplate(
        "widgets/z-progressbar",
        "z-progressbar-base-tpl");

    this.appendChild(tpl);

    if (this.hasAttribute('data-label')) {
      this.renderDataLabel();
    }

     if (this.hasAttribute('data-progress')) {
      this.updateProgressBar(this.getAttribute('data-progress'));
    }

    this.attributeChangedCallback();
  };

  this.attributeChangedCallback = function(attr, old_val, new_val) {
    switch (attr) {
      case "data-progress":
        this.updateProgressBar(new_val);
        break;
      case "data-label":
        this.renderDataLabel();
        break;
      default:
        break;
    }
  };

  this.renderDataLabel = function() {
    var label_elem = this.querySelector(".label");
    if (!label_elem) {
      label_elem = document.createElement("div");
      label_elem.className = "label";
      this.appendChild(label_elem);
    }

    label_elem.innerHTML = this.getAttribute('data-label');
  };

  this.updateProgressBar = function(progress) {
    var bar_elem = this.querySelector(".bar");
    bar_elem.style.width = progress + "%";

    if (!(this.getAttribute('data-show-progress') == "off")) {
      var progress_elem = this.querySelector(".progress");
      progress_elem.innerHTML = parseFloat(progress).toFixed(1) + "%";
    }
  };
};

var proto = Object.create(HTMLElement.prototype);
ProgressBarComponent.apply(proto);
document.registerElement("z-progressbar", { prototype: proto });

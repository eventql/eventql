/**
  This file is part of the "FnordMetric" project
    Copyright (c) 2015 Laura Schlimmer
    Copyright (c) 2015 Paul Asmuth

  FnordMetric is free software: you can redistribute it and/or modify it under
  the terms of the GNU General Public License v3.0. You should have received a
  copy of the GNU General Public License along with this program. If not, see
  <http://www.gnu.org/licenses/>.
*/

var ZChartComponent = function() {
  this.createdCallback = function() {
    console.log("z chart component");
  };

};

var proto = Object.create(HTMLElement.prototype);
ZChartComponent.apply(proto);
document.registerElement("z-chart", { prototype: proto });

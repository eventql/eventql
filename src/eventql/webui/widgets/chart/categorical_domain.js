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

EventQL.ChartPlotter = this.EventQL.ChartPlotter || {};

EventQL.ChartPlotter.CategoricalDomain = function(opts) {
  'use strict';

  var categories = [];

  this.addCategory = function(category) {
    if (!ArrayUtil.contains(categories, category)) {
      categories.push(category);
    }
  };

  this.convertDomainToScreen = function(value) {
    var index = categories.length - categories.indexOf(value);

    if (index < 1) {
      //throw new Error("can't scale value");
    }

    var cardinality = categories.length;
    var scaled = (index - 0.5) / cardinality;
    return scaled;
  };

  this.getLabels = function() {
    var labels = [];

    for (var i = 0; i < categories.length; i++) {
      var point = this.convertDomainToScreen(categories[i]);
      labels.push([point, categories[i]]);
    }

    return labels;
  };

};




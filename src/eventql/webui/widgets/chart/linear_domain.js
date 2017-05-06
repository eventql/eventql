/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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

EventQL.ChartPlotter.ContinuousDomain = function(opts) {
  'use strict';

  const DEFAULT_NUM_TICKS = 12;

  var min = null;
  var max = null;

  this.findMinMax = function(values) {
    this.setMin(Math.min.apply(null, values));
    this.setMax(Math.max.apply(null, values));
  }

  this.setMin = function(min_value) {
    if (isNaN(min_value)) {
      throw new Error();
    }

    min = Math.min(min, min_value);
  }

  this.setMax = function(max_value) {
    if (isNaN(max_value)) {
      throw new Error();
    }

    max = Math.max(max, max_value);
  }

  this.convertDomainToScreen = function(input_val) {
    if (input_val < min) {
      return 0;
    }

    if (input_val > max) {
      return 1.0
    }

    return (input_val - min) / (max - min);
  }

  this.convertScreenToDomain = function(input_val) {
    if (input_val < 0) {
      input_val = 0;
    }

    if (input_val > 1.0) {
      input_val = 1.0;
    }

    return input_val * (max - min) + min;
  }

  this.getTicks = function() {
    var ticks = [];
    for (var i = 0; i < DEFAULT_NUM_TICKS; i++) {
      ticks.push(i / DEFAULT_NUM_TICKS);
    }

    return ticks;
  };

  this.getLabels = function() {
    var ticks = this.getTicks();
    var labels = [];

    for (var i = 0; i < ticks.length; i++) {
      //TODO format
      labels.push(ticks[i], this.convertScreenToDomain(ticks[i]));
    }

    return labels;
  };

};


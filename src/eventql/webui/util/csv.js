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

CSVUtil = this.CSVUtil || {};

CSVUtil.toCSV = function(opts, columns, rows) {
  opts = opts || {};
  opts.row_separator = opts.row_separator || "\n";
  opts.column_separator = opts.column_separator || ",";
  opts.quote_char = opts.quote_char || "\"";

  var csv = "";
  var add_row = function(row) {
    var r = row.map(function(s) {
      s = (s || "").toString();

      while (s.indexOf(opts.quote_char) >= 0) {
        s = s.replace(opts.quote_char, "\\" + opts.quote_char);
      }

      return opts.quote_char + s + opts.quote_char;
    })

    csv += r.join(opts.column_separator) + opts.row_separator;
  }

  add_row(columns);

  rows.forEach(function(row) {
    var r = [];
    for (var i = 0; i < columns.length; ++i) {
      var cell = row[i];
      r.push(cell ? cell : null);
    }
    add_row(r);
    });

    return csv;
};


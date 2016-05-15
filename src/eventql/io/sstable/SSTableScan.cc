/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <algorithm>
#include <eventql/io/sstable/SSTableScan.h>
#include <eventql/io/sstable/SSTableColumnReader.h>


namespace sstable {

SSTableScan::SSTableScan(
    SSTableColumnSchema* schema) :
    schema_(schema),
    limit_(-1),
    offset_(0),
    has_order_by_(false) {
  if (schema_) {
    select_list_.emplace_back(0);
    auto col_ids = schema->columnIDs();
    for (const auto& c : col_ids) {
      select_list_.emplace_back(c);
    }
  }
}

void SSTableScan::setKeyPrefix(const String& prefix) {
  setKeyFilterRegex(prefix + ".*"); // FIXPAUL HACK !!! ;) :) ;)
}

void SSTableScan::setKeyFilterRegex(const String& regex) {
  key_filter_regex_ = Some(std::regex(regex));
}

void SSTableScan::setKeyExactMatchFilter(const String& str) {
  Set<String> match_set;
  match_set.emplace(str);
  setKeyExactMatchFilter(match_set);
}

void SSTableScan::setKeyExactMatchFilter(const Set<String>& match_set) {
  key_exact_match_ = match_set;
}

void SSTableScan::setLimit(long int limit) {
  limit_ = limit;
}

void SSTableScan::setOffset(long unsigned int offset) {
  offset_ = offset;
}

void SSTableScan::setOrderBy(const String& column, const String& order_fn) {
  if (order_fn == "STRASC") {
    setOrderBy(column, [] (const String& a, const String& b) {
      return a < b;
    });

    return;
  }

  if (order_fn == "STRDSC") {
    setOrderBy(column, [] (const String& a, const String& b) {
      return b < a;
    });

    return;
  }

  if (order_fn == "NUMASC") {
    setOrderBy(column, [] (const String& a, const String& b) {
      return std::stod(a) < std::stod(b);
    });

    return;
  }

  if (order_fn == "NUMDSC") {
    setOrderBy(column, [] (const String& a, const String& b) {
      return std::stod(b) < std::stod(a);
    });

    return;
  }

  RAISEF(
      kIllegalArgumentError,
      "invalid order fn: $0, valid arguments: STRASC, STRDSC, NUMASC, NUMDSC",
      order_fn);
}

void SSTableScan::setOrderBy(const String& column, OrderFn order_fn) {
  if (!schema_) {
    RAISE(kIllegalStateError, "requires a sstable schema");
  }

  has_order_by_ = true;
  order_by_fn_ = order_fn;

  auto colid = schema_->columnID(column);

  for (int i = 0; i < select_list_.size(); ++i) {
    if (select_list_[i] == colid) {
      order_by_index_ = i;
      return;
    }
  }

  RAISE(
      kIllegalArgumentError,
      "the order_by column must be included in the select list");
}

Vector<String> SSTableScan::columnNames() const {
  if (!schema_) {
    RAISE(kIllegalStateError, "requires a sstable schema");
  }

  Vector<String> cols;

  for (const auto& s : select_list_) {
    cols.emplace_back(s == 0 ? "_key" : schema_->columnName(s));
  }

  return cols;
}

void SSTableScan::execute(
    Cursor* cursor,
    Function<void (const Vector<String> row)> fn) {
  Vector<Vector<String>> rows;
  size_t limit_ctr = 0;
  size_t offset_ctr = 0;

  for (; cursor->valid(); cursor->next()) {
    auto key = cursor->getKeyString();

    if (key_exact_match_.size() > 0 && key_exact_match_.count(key) == 0) {
      continue;
    }

    if (!key_filter_regex_.isEmpty()) {
      if (!std::regex_match(key, key_filter_regex_.get())) {
        continue;
      }
    }

    Vector<String> row;
    if (schema_) {
      auto val = cursor->getDataBuffer();
      sstable::SSTableColumnReader cols(schema_, val);

      for (const auto& s : select_list_) {
        switch (s) {
          case 0:
            row.emplace_back(cursor->getKeyString());
            break;

          default:
            row.emplace_back(cols.getStringColumn(s));
            break;
        }
      }
    } else {
      row.emplace_back(cursor->getKeyString());
      row.emplace_back(cursor->getDataString());
    }

    // filter cols...

    if (!has_order_by_ && offset_ctr++ < offset_) {
      continue;
    }

    if (has_order_by_) {
      rows.emplace_back(row);
    } else {
      fn(row);

      if (limit_ > 0 && ++limit_ctr >= limit_) {
        break;
      }
    }
  }

  if (has_order_by_) {
    std::sort(rows.begin(), rows.end(), [this] (
        const Vector<String>& a,
        const Vector<String>& b) {
      return order_by_fn_(a[order_by_index_], b[order_by_index_]);
    });

    auto limit = rows.size();
    if (limit_ > 0) {
      limit = offset_ + limit_;

      if (limit > rows.size()) {
        limit = rows.size();
      }
    }

    for (int i = offset_; i < limit; ++i) {
      fn(rows[i]);
    }
  }
}

} // namespace sstable


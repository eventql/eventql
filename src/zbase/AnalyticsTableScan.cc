/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/AnalyticsTableScan.h"

using namespace stx;

namespace zbase {

AnalyticsTableScan::AnalyticsTableScan() : rows_scanned_(0) {}

void AnalyticsTableScan::scanTable(cstable::CSTableReader* reader) {
  per_session_.clear();
  per_query_.clear();
  per_queryitem_.clear();
  per_cartitem_.clear();
  per_itemvisit_.clear();

  for (const auto& c : columns_) {
    String colname =
        StringUtil::find(c.first, '.') == String::npos
            ? "attr." + c.first
            : "event." + c.first;

    if (!reader->hasColumn(colname)) {
      continue;
    }

    if (StringUtil::beginsWith(colname, "event.cart_items.")) {
      per_cartitem_.emplace_back(
          c.second.get(),
          reader->getColumnReader(colname));
      continue;
    }

    if (StringUtil::beginsWith(colname, "event.search_queries.result_items.")) {
      per_queryitem_.emplace_back(
          c.second.get(),
          reader->getColumnReader(colname));
      continue;
    }

    if (StringUtil::beginsWith(colname, "event.search_queries.")) {
      per_query_.emplace_back(
          c.second.get(),
          reader->getColumnReader(colname));
      continue;
    }

    if (StringUtil::beginsWith(colname, "event.item_visits.")) {
      per_itemvisit_.emplace_back(
          c.second.get(),
          reader->getColumnReader(colname));
      continue;
    }

    per_session_.emplace_back(
        c.second.get(),
        reader->getColumnReader(colname));
  }

  if (per_queryitem_.size() > 0) {
    scanTableForQueryItems(reader->numRecords());
    return;
  }

  if (per_query_.size() > 0) {
    scanTableForQueries(reader->numRecords());
    return;
  }

  if (per_cartitem_.size() > 0) {
    scanTableForCartItems(reader->numRecords());
    return;
  }

  if (per_itemvisit_.size() > 0) {
    scanTableForItemVisits(reader->numRecords());
    return;
  }

  RAISE(kIllegalStateError, "can't figure out how to run this query");
}

void AnalyticsTableScan::scanTableForQueryItems(size_t n_max) {
  uint64_t d;
  uint64_t r;

  for (size_t n = 1; n < n_max; ) {
    rows_scanned_++;

    bool is_defined = false;
    for (const auto& c : per_queryitem_) {
      c.second->next(&r, &d, &c.first->cur_data, &c.first->cur_size);
      if (d == c.second->maxDefinitionLevel()) {
        is_defined = true;
      }
    }

    switch (r) {
      case 0:
        ++n;

        for (const auto& c : per_session_) {
          c.second->next(&r, &d, &c.first->cur_data, &c.first->cur_size);
        }

        for (const auto& f : on_session_) {
          f();
        }
        /* fallthrough */

      case 1:
        for (const auto& c : per_query_) {
          c.second->next(&r, &d, &c.first->cur_data, &c.first->cur_size);
        }

        for (const auto& f : on_query_) {
          f();
        }
        /* fallthrough */

      case 2:
        if (is_defined) {
          for (const auto& f : on_queryitem_) {
            f();
          }
        }

        break;
    }
  }
}

void AnalyticsTableScan::scanTableForQueries(size_t n_max) {
  uint64_t d;
  uint64_t r;

  for (size_t n = 1; n < n_max; ) {
    rows_scanned_++;

    bool is_defined = false;
    for (const auto& c : per_query_) {
      c.second->next(&r, &d, &c.first->cur_data, &c.first->cur_size);
      if (d == c.second->maxDefinitionLevel()) {
        is_defined = true;
      }
    }

    switch (r) {
      case 0:
        ++n;

        for (const auto& c : per_session_) {
          c.second->next(&r, &d, &c.first->cur_data, &c.first->cur_size);
        }

        for (const auto& f : on_session_) {
          f();
        }
        /* fallthrough */

      case 1:
        if (is_defined) {
          for (const auto& f : on_query_) {
            f();
          }
        }
        break;

    }
  }
}

void AnalyticsTableScan::scanTableForCartItems(size_t n_max) {
  uint64_t d;
  uint64_t r;

  for (size_t n = 1; n < n_max; ) {
    rows_scanned_++;

    bool is_defined = false;
    for (const auto& c : per_cartitem_) {
      c.second->next(&r, &d, &c.first->cur_data, &c.first->cur_size);
      if (d == c.second->maxDefinitionLevel()) {
        is_defined = true;
      }
    }

    switch (r) {
      case 0:
        ++n;

        for (const auto& c : per_session_) {
          c.second->next(&r, &d, &c.first->cur_data, &c.first->cur_size);
        }

        for (const auto& f : on_session_) {
          f();
        }
        /* fallthrough */

      case 1:
        if (is_defined) {
          for (const auto& f : on_cartitem_) {
            f();
          }
        }
        break;

    }
  }
}

void AnalyticsTableScan::scanTableForItemVisits(size_t n_max) {
  uint64_t d;
  uint64_t r;

  for (size_t n = 1; n < n_max; ) {
    rows_scanned_++;

    bool is_defined = false;
    for (const auto& c : per_itemvisit_) {
      c.second->next(&r, &d, &c.first->cur_data, &c.first->cur_size);
      if (d == c.second->maxDefinitionLevel()) {
        is_defined = true;
      }
    }

    switch (r) {
      case 0:
        ++n;

        for (const auto& c : per_session_) {
          c.second->next(&r, &d, &c.first->cur_data, &c.first->cur_size);
        }

        for (const auto& f : on_session_) {
          f();
        }
        /* fallthrough */

      case 1:
        if (is_defined) {
          for (const auto& f : on_itemvisit_) {
            f();
          }
        }
        break;

    }
  }
}



void AnalyticsTableScan::onSession(Function<void ()> fn) {
  on_session_.emplace_back(fn);
}

void AnalyticsTableScan::onQuery(Function<void ()> fn) {
  on_query_.emplace_back(fn);
}

void AnalyticsTableScan::onQueryItem(Function<void ()> fn) {
  on_queryitem_.emplace_back(fn);
}

void AnalyticsTableScan::onCartItem(Function<void ()> fn) {
  on_cartitem_.emplace_back(fn);
}

void AnalyticsTableScan::onItemVisit(Function<void ()> fn) {
  on_itemvisit_.emplace_back(fn);
}

RefPtr<AnalyticsTableScan::ColumnRef> AnalyticsTableScan::fetchColumn(
    const String& column_name) {
  auto iter = columns_.find(column_name);
  if (iter == columns_.end()) {
    RefPtr<ColumnRef> cr(new ColumnRef(column_name));
    columns_.emplace(column_name, cr);
    return cr;
  } else {
    return iter->second;
  }
}

size_t AnalyticsTableScan::rowsScanned() const {
  return rows_scanned_;
}

AnalyticsTableScan::ColumnRef::ColumnRef(
    const String& name) :
    column_name(name),
    cur_data(nullptr),
    cur_size(0) {}

uint32_t AnalyticsTableScan::ColumnRef::getUInt32() const {
  if (cur_size == sizeof(uint64_t)) {
    return *((uint64_t*) cur_data);
  }

  if (cur_size < sizeof(uint32_t)) {
    return 0;
  }

  return *((uint32_t*) cur_data);
}

uint64_t AnalyticsTableScan::ColumnRef::getUInt64() const {
  if (cur_size == sizeof(uint32_t)) {
    return *((uint64_t*) cur_data);
  }

  if (cur_size < sizeof(uint64_t)) {
    return 0;
  }

  return *((uint64_t*) cur_data);
}

bool AnalyticsTableScan::ColumnRef::getBool() const {
  if (cur_size < sizeof(uint8_t)) {
    return false;
  }

  return *((uint8_t*) cur_data) > 0;
}

String AnalyticsTableScan::ColumnRef::getString() const {
  if (cur_size == 0) {
    return "";
  }

  return String((char*) cur_data, cur_size);
}

} // namespace zbase


/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ANALYTICSTABLESCAN_H
#define _CM_ANALYTICSTABLESCAN_H
#include <stx/stdtypes.h>
#include "cstable/CSTableReader.h"
#include "cstable/CSTableReader.h"
#include "cstable/ColumnReader.h"
#include "cstable/BitPackedIntColumnReader.h"
#include "dproc/Task.h"
#include "zbase/CTRCounter.h"

using namespace stx;

namespace cm {

class AnalyticsTableScan {
public:

  struct ColumnRef : public RefCounted {
    ColumnRef(const String& name);

    const String column_name;
    void* cur_data;
    size_t cur_size;

    uint32_t getUInt32() const;
    uint64_t getUInt64() const;
    bool getBool() const;
    String getString() const;
  };

  AnalyticsTableScan();
  void scanTable(cstable::CSTableReader* reader);

  RefPtr<ColumnRef> fetchColumn(const String& column_name);

  void onSession(Function<void ()> fn);
  void onQuery(Function<void ()> fn);
  void onQueryItem(Function<void ()> fn);
  void onCartItem(Function<void ()> fn);
  void onItemVisit(Function<void ()> fn);

  size_t rowsScanned() const;

protected:

  void scanTableForQueryItems(size_t n_max);
  void scanTableForQueries(size_t n_max);
  void scanTableForCartItems(size_t n_max);
  void scanTableForItemVisits(size_t n_max);

  HashMap<String, RefPtr<ColumnRef>> columns_;
  List<Function<void ()>> on_session_;
  List<Function<void ()>> on_query_;
  List<Function<void ()>> on_queryitem_;
  List<Function<void ()>> on_cartitem_;
  List<Function<void ()>> on_itemvisit_;
  Vector<Pair<ColumnRef*, RefPtr<cstable::ColumnReader>>> per_session_;
  Vector<Pair<ColumnRef*, RefPtr<cstable::ColumnReader>>> per_query_;
  Vector<Pair<ColumnRef*, RefPtr<cstable::ColumnReader>>> per_queryitem_;
  Vector<Pair<ColumnRef*, RefPtr<cstable::ColumnReader>>> per_cartitem_;
  Vector<Pair<ColumnRef*, RefPtr<cstable::ColumnReader>>> per_itemvisit_;

  size_t rows_scanned_;
};

} // namespace cm

#endif

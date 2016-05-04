/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRCOUNTER_H
#define _CM_CTRCOUNTER_H
#include "stx/stdtypes.h"
#include "stx/option.h"
#include "stx/json/json.h"
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/infra/sstable/SSTableEditor.h"
#include "eventql/infra/sstable/SSTableColumnSchema.h"
#include "eventql/infra/sstable/SSTableColumnReader.h"
#include "eventql/infra/sstable/SSTableColumnWriter.h"

using namespace stx;

namespace zbase {

struct CTRCounterData {
  static CTRCounterData load(const String& buf);
  static sstable::SSTableColumnSchema sstableSchema();

  CTRCounterData();
  void merge(const CTRCounterData& other);
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
  void toJSON(json::JSONOutputStream* json) const;

  uint64_t num_views;
  uint64_t num_clicked;
  uint64_t num_clicks;
  uint64_t gmv_eurcent;
  uint64_t cart_value_eurcent;
};

typedef Pair<String, CTRCounterData> CTRCounter;

}
#endif

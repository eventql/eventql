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
#include "fnord-base/stdtypes.h"
#include "fnord-base/option.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"

using namespace fnord;

namespace cm {

struct CTRCounterData {
  static CTRCounterData load(const String& buf);
  static sstable::SSTableColumnSchema sstableSchema();

  CTRCounterData();
  void merge(const CTRCounterData& other);
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);

  uint64_t num_views;
  uint64_t num_clicked;
  uint64_t num_clicks;
};

typedef Pair<String, CTRCounterData> CTRCounter;

}
#endif

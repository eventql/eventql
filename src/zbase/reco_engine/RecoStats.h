/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_RECOSTATS_H
#define _CM_RECOSTATS_H
#include <stx/stdtypes.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/json/json.h>

using namespace stx;

namespace zbase {

struct RecoStats {
  RecoStats();

  uint64_t num_queries;
  uint64_t num_queries_with_recos;
  uint64_t num_queries_with_recos_seen;
  uint64_t num_queries_with_recos_clicked;
  uint64_t num_reco_items;
  uint64_t num_reco_items_seen;
  uint64_t num_reco_items_clicked;

  void merge(const RecoStats& other);
  void toJSON(json::JSONOutputStream* json) const;
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
};

} // namespace zbase

#endif

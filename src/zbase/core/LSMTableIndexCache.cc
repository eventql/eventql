/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/io/fileutil.h>
#include <stx/logging.h>
#include <zbase/core/LSMTableIndexCache.h>

using namespace stx;

namespace zbase {

LSMTableIndexCache::LSMTableIndexCache(
    const String& base_path) :
    base_path_(base_path) {}

RefPtr<LSMTableIndex> LSMTableIndexCache::lookup(const String& filename) {
  ScopedLock<std::mutex> lk(mutex_);
  auto& slot = map_[filename];
  if (!slot.get()) {
    slot = new LSMTableIndex();
  }

  auto idx = slot;
  lk.unlock();

  idx->load(FileUtil::joinPaths(base_path_, filename + ".idx"));
  return idx;
}


} // namespace zbase


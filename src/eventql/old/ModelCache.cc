/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/logging.h"
#include "stx/exception.h"
#include "stx/io/fileutil.h"
#include "ModelCache.h"

using namespace stx;

namespace zbase {

ModelCache::ModelCache(const String& datadir) : datadir_(datadir) {}

RefCounted* ModelCache::getModel(
    const String& factory_name,
    const String& index_name,
    const String& prefix) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto key = index_name + "~" + prefix;
  auto cached = models_.find(key);
  if (cached == models_.end()) {
    return loadModel(factory_name, index_name, prefix, false);
  } else {
    return cached->second;
  }
}

void ModelCache::addModelFactory(
    const String& factory_name,
    Function<RefCounted* (const String& filename)> fn) {
  factories_[factory_name] = fn;
}

RefCounted* ModelCache::loadModel(
    const String& factory_name,
    const String& index_name,
    const String& prefix,
    bool lock) {
  auto factory = factories_.find(factory_name);
  if (factory == factories_.end()) {
    RAISEF(kRuntimeError, "invalid model factory: '$0'", factory_name);
  }

  auto filename = getLatestModelFilename(index_name, prefix);

  stx::logInfo(
      "cm.modelcache",
      "Loading new model $0/$1 from $2",
      index_name,
      prefix,
      filename);

  auto model = factory->second(filename);
  model->incRef();

  std::unique_lock<std::mutex> lk(mutex_, std::defer_lock);
  if (lock) {
    lk.lock();
  }

  auto key = index_name + "~" + prefix;
  auto cached = models_.find(key);
  if (cached == models_.end()) {
    models_.emplace(key, model);
  } else {
    cached->second->decRef();
    cached->second = model;
  }

  return model;
}

String ModelCache::getLatestModelFilename(
    const String& index_name,
    const String& prefix) {
  auto artifacts = getArtifactIndex(index_name);
  auto artifactlist = artifacts->listArtifacts();

  String filename;
  uint64_t built_at = 0;

  for (const auto& a : artifactlist) {
    if (a.files.size() != 1 ||
        !StringUtil::beginsWith(a.name, prefix)) {
      continue;
    }

    String this_built_at;
    for (const auto& a : a.attributes) {
      if (a.first == "built_at") {
        this_built_at = a.second;
        break;
      }
    }

    if (this_built_at.length() == 0) {
      continue;
    }

    auto this_filename = FileUtil::joinPaths(datadir_, a.files[0].filename);
    if (!FileUtil::exists(this_filename)) {
      continue;
    }

    auto this_built_at_i = std::stoull(this_built_at);
    if (this_built_at_i > built_at) {
      filename = this_filename;
      built_at = this_built_at_i;
    }
  }

  if (built_at == 0) {
    RAISEF(kRuntimeError, "no model found for: $0/$1", index_name, prefix);
  }

  return filename;
}

RefPtr<logtable::ArtifactIndex> ModelCache::getArtifactIndex(
    const String& index_name) {
  auto iter = artifacts_.find(index_name);
  if (iter != artifacts_.end()) {
    return iter->second;
  }

  RefPtr<logtable::ArtifactIndex> index(
      new logtable::ArtifactIndex(datadir_, index_name, true));

  artifacts_.emplace(index_name, index);
  return index;
}

} // namespace zbase


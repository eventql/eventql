/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "IndexBuild.h"

using namespace fnord;

namespace cm {

IndexBuild::IndexBuild(
    FeatureIndexWriter* feature_idx) :
    feature_idx_(feature_idx) {}

void IndexBuild::updateDocument(const IndexRequest& index_request) {
  fnord::iputs("update...", 1);
}

void IndexBuild::commit() {
  fnord::iputs("commit...", 1);
}

} // namespace cm

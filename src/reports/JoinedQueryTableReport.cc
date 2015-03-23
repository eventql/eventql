/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/JoinedQueryTableReport.h"

using namespace fnord;

namespace cm {

JoinedQueryTableReport::JoinedQueryTableReport(
    const Set<String>& sstable_filenames) :
    input_files_(sstable_filenames) {}

void JoinedQueryTableReport::addReport(
    RefPtr<Report> report,
    Function<void (const cm::JoinedQuery& q)> on_query) {
}

void JoinedQueryTableReport::build() {
}

Set<String> JoinedQueryTableReport::inputFiles() {
  return input_files_;
}

Set<String> JoinedQueryTableReport::outputFiles() {
 return Set<String>();
}

} // namespace cm


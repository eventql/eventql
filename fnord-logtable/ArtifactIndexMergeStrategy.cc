/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-logtable/ArtifactIndexMergeStrategy.h>

namespace fnord {
namespace logtable {

void AppendOnlyMergeStrategy::merge(
    ArtifactIndexSnapshot* local,
    const ArtifactIndexSnapshot* remote) const {
  Set<String> existing_artifacts;

  for (const auto& a : local->artifacts) {
    existing_artifacts.emplace(a.name);
  }

  for (const auto& a : remote->artifacts) {
    if (existing_artifacts.count(a.name) > 0) {
      continue;
    }

    auto new_artifact = a;
    new_artifact.status = ArtifactStatus::DOWNLOAD;
    local->artifacts.emplace_back(new_artifact);
  }
}

} // namespace logtable
} // namespace fnord


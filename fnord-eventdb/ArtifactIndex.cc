/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/exception.h>
#include <fnord-base/logging.h>
#include <fnord-base/io/FileLock.h>
#include <fnord-eventdb/ArtifactIndex.h>

namespace fnord {
namespace eventdb {

ArtifactIndex::ArtifactIndex(
    const String& db_path,
    const String& index_name,
    bool readonly) :
    db_path_(db_path),
    index_name_(index_name),
    readonly_(readonly) {}

void ArtifactIndex::addArtifact(const ArtifactRef& artifact) {
  fnord::logDebug("fn.evdb", "Adding artifact: $0", artifact.name);

  FileLock lk(StringUtil::format("$0/$1.idx.lck", db_path_, index_name_));
  lk.lock(true);

  auto index = readIndex();

  for (const auto& a : index) {
    if (a.name == artifact.name) {
      RAISEF(kIndexError, "artifact '$0' already exists in index", a.name);
    }
  }

  index.emplace_back(artifact);
  writeIndex(index);
}

void ArtifactIndex::updateStatus(
    const String& artifact_name,
    ArtifactStatus new_status) {
  FileLock lk(StringUtil::format("$0/$1.idx.lck", db_path_, index_name_));
  lk.lock(true);

  auto index = readIndex();

  for (auto& a : index) {
    if (a.name == artifact_name) {
      statusTransition(&a, new_status);
      writeIndex(index);
      return;
    }
  }

  RAISEF(kIndexError, "artifact '$0' not found", artifact_name);
}

void ArtifactIndex::statusTransition(
    ArtifactRef* artifact,
    ArtifactStatus new_status) {
  artifact->status = new_status; // FIXPAUL proper transition
}

List<ArtifactRef> ArtifactIndex::readIndex() const {
  List<ArtifactRef> index;

  return index;
}

void ArtifactIndex::writeIndex(const List<ArtifactRef>& index) {
}

} // namespace eventdb
} // namespace fnord


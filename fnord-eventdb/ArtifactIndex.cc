/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/logging.h>
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
}

void ArtifactIndex::updateStatus(
    const String& artifact_name,
    ArtifactStatus new_status) {

}

} // namespace eventdb
} // namespace fnord


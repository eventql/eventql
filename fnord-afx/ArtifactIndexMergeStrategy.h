/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_ARTIFACTINDEXMERGESTRATEGY_H
#define _FNORD_LOGTABLE_ARTIFACTINDEXMERGESTRATEGY_H
#include <fnord-base/stdtypes.h>
#include <fnord-afx/ArtifactIndex.h>

namespace fnord {
namespace logtable {

class ArtifactIndexMergeStrategy : public RefCounted {
public:
  virtual ~ArtifactIndexMergeStrategy() {}

  /* both indexes will be locked while the mergeop is running */
  virtual void merge(
      ArtifactIndexSnapshot* local,
      const ArtifactIndexSnapshot* remote) const = 0;

};

class AppendOnlyMergeStrategy : public ArtifactIndexMergeStrategy {
public:
  void merge(
      ArtifactIndexSnapshot* local,
      const ArtifactIndexSnapshot* remote) const override;
};

} // namespace logtable
} // namespace fnord

#endif

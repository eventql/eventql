/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <fnord/autoref.h>
#include <dproc/Task.h>

using namespace fnord;

namespace dproc {

class TaskRef : public RefCounted {
public:

  virtual RefPtr<RDD> getInstance() const = 0;

  template <typename RDDType>
  RefPtr<RDDType> getInstanceAs() const;

  virtual RefPtr<VFSFile> getData() const = 0;

};

class DiskTaskRef : public TaskRef {
public:
  typedef Function<RefPtr<RDD> ()> InstanceFactoryFn;

  DiskTaskRef(
      const String& filename,
      const InstanceFactoryFn factory);

  RefPtr<RDD> getInstance() const override;

  RefPtr<VFSFile> getData() const override;

protected:
  String filename_;
};

class LiveTaskRef : public TaskRef {
public:

  LiveTaskRef(RefPtr<RDD> instance);

  RefPtr<RDD> getInstance() const override;

  RefPtr<VFSFile> getData() const override;

protected:
  RefPtr<RDD> instance_;
};

} // namespace dproc


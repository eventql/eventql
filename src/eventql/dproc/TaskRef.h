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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/io/file.h>
#include <eventql/dproc/Task.h>

using namespace stx;

namespace dproc {

class TaskRef : public RefCounted {
public:

  virtual RefPtr<RDD> getInstance() const = 0;

  template <typename RDDType>
  RefPtr<RDDType> getInstanceAs() const;

  virtual RefPtr<VFSFile> getData() const = 0;

  virtual String contentType() const = 0;

  virtual void saveToFile(const String& filename) const = 0;

};

class DiskTaskRef : public TaskRef {
public:
  typedef Function<RefPtr<RDD> ()> InstanceFactoryFn;

  DiskTaskRef(
      const String& filename,
      const InstanceFactoryFn factory,
      const String& content_type);

  RefPtr<RDD> getInstance() const override;

  RefPtr<VFSFile> getData() const override;

  virtual String contentType() const override;

  void saveToFile(const String& filename) const override;

protected:
  String filename_;
  InstanceFactoryFn factory_;
  String content_type_;
};

class LiveTaskRef : public TaskRef {
public:

  LiveTaskRef(RefPtr<RDD> instance);

  RefPtr<RDD> getInstance() const override;

  RefPtr<VFSFile> getData() const override;

  virtual String contentType() const override;

  void saveToFile(const String& filename) const override;

protected:
  RefPtr<RDD> instance_;
};

template <typename TaskType>
RefPtr<TaskType> TaskRef::getInstanceAs() const {
  auto instance = getInstance();

  auto cast = dynamic_cast<TaskType*>(instance.get());
  if (cast == nullptr) {
    RAISE(kTypeError, "can't make TaskRef into requested TaskType");
  }

  return mkRef(cast);
}

} // namespace dproc


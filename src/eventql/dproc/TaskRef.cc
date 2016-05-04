/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/dproc/TaskRef.h>
#include <eventql/util/io/mmappedfile.h>
#include <eventql/util/io/fileutil.h>

using namespace stx;

namespace dproc {

DiskTaskRef::DiskTaskRef(
    const String& filename,
    const InstanceFactoryFn factory,
    const String& content_type) :
    filename_(filename),
    factory_(factory),
    content_type_(content_type) {}

RefPtr<RDD> DiskTaskRef::getInstance() const {
  auto instance = factory_();
  instance->decode(getData());
  return instance;
}

RefPtr<VFSFile> DiskTaskRef::getData() const {
  return new io::MmappedFile(File::openFile(filename_, File::O_READ));
}

void DiskTaskRef::saveToFile(const String& filename) const {
  FileUtil::cp(filename_, filename);
}

String DiskTaskRef::contentType() const {
  return content_type_;
}

LiveTaskRef::LiveTaskRef(RefPtr<RDD> instance) : instance_(instance) {}

RefPtr<RDD> LiveTaskRef::getInstance() const {
  return instance_;
}

RefPtr<VFSFile> LiveTaskRef::getData() const {
  return instance_->encode();
}

String LiveTaskRef::contentType() const {
  return instance_->contentType();
}

void LiveTaskRef::saveToFile(const String& filename) const {
  auto file = File::openFile(
      filename,
      File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

  auto data = getData();
  file.write(data->data(), data->size());
}

} // namespace dproc


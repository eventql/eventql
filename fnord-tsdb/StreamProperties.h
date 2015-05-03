/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_STREAMPROPERTIES_H
#define _FNORD_TSDB_STREAMPROPERTIES_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/option.h>
#include <fnord-msg/MessageSchema.h>

namespace fnord {
namespace tsdb {

struct StreamProperties : public RefCounted {
  StreamProperties(
      RefPtr<msg::MessageSchema> _schema) :
      schema(_schema),
      chunk_size(kMicrosPerSecond * 3600),
      max_datafile_size(1024 * 1024 * 128) {}

  StreamProperties(const StreamProperties& other) :
      schema(other.schema),
      chunk_size(other.chunk_size),
      max_datafile_size(other.max_datafile_size) {}

  RefPtr<msg::MessageSchema> schema;
  Duration chunk_size;
  size_t max_datafile_size;
};

}
}
#endif

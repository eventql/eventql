/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_CSTABLEBUILDER_H
#define _FNORD_CSTABLE_CSTABLEBUILDER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/io/file.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-base/autoref.h>
#include <fnord-cstable/ColumnWriter.h>
#include <fnord-msg/MessageSchema.h>

namespace fnord {
namespace cstable {

class CSTableBuilder {
public:
  CSTableBuilder(
      msg::MessageSchema* schema,
      HashMap<String, RefPtr<ColumnWriter>> column_writers);

  void addRecord(const Buffer& buf);

  void write(const String& filename);

protected:
  msg::MessageSchema* schema_;
  HashMap<String, RefPtr<ColumnWriter>> columns_;
};

} // namespace cstable
} // namespace fnord

#endif

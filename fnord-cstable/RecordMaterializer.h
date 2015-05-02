/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_RECORDMATERIALIZER_H
#define _FNORD_CSTABLE_RECORDMATERIALIZER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/io/file.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-cstable/BinaryFormat.h>
#include <fnord-cstable/ColumnReader.h>

namespace fnord {
namespace cstable {

class RecordMaterializer {
public:

  RecordMaterializer(
      RefPtr<msg::MessageSchema> schema,
      CSTableReader* reader);

  void nextRecord(msg::MessageObject* record);
  void skipRecord();

protected:
  //HashMap<String, Column> columns_;
};

} // namespace cstable
} // namespace fnord

#endif

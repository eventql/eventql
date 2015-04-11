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
#include <fnord-msg/MessageBuilder.h>

namespace fnord {
namespace cstable {

class CSTableBuilder {
public:
  CSTableBuilder(
      msg::MessageSchema* schema,
      HashMap<String, RefPtr<ColumnWriter>> column_writers);

  void addRecord(const msg::MessageBuilder& msg);

  void write(const String& filename);

protected:

  void addRecordField(
      uint32_t r,
      uint32_t rmax,
      uint32_t d,
      const msg::MessageBuilder& msg,
      const String& column,
      const String& path,
      const msg::MessageSchemaField& field);

  void writeNull(
      uint32_t r,
      uint32_t d,
      const String& column,
      const msg::MessageSchemaField& field);

  void writeField(
      uint32_t r,
      uint32_t d,
      const msg::MessageBuilder& msg,
      const String& column,
      const String& path,
      const msg::MessageSchemaField& field);

  msg::MessageSchema* schema_;
  HashMap<String, RefPtr<ColumnWriter>> columns_;
};

} // namespace cstable
} // namespace fnord

#endif

/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_TABLECHUNKSUMMARYWRITER_H
#define _FNORD_LOGTABLE_TABLECHUNKSUMMARYWRITER_H
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/random.h>
#include <stx/io/FileLock.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/protobuf/MessageSchema.h>
#include <stx/protobuf/MessageObject.h>

namespace stx {
namespace logtable {

class TableChunkSummaryWriter {
public:

  TableChunkSummaryWriter(const String& output_filename);

  void addSummary(
      const String& summary_name,
      void* data,
      size_t size);

  void commit();

protected:
  String output_filename_;
  util::BinaryMessageWriter head_;
  util::BinaryMessageWriter body_;
  uint32_t n_;
};

}
}
#endif

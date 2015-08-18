/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_NUMERICBOUNDSSUMMARYBUILDER_H
#define _FNORD_LOGTABLE_NUMERICBOUNDSSUMMARYBUILDER_H
#include <stx/stdtypes.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/util/binarymessagereader.h>
#include <fnord-logtable/TableChunkSummaryBuilder.h>

namespace stx {
namespace logtable {
class TableChunkSummaryWriter;

struct NumericBoundsSummary {
  double min_value;
  double max_value;

  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
};

class NumericBoundsSummaryBuilder : public TableChunkSummaryBuilder {
public:

  NumericBoundsSummaryBuilder(
      const String& summary_name,
      uint32_t field_id);

  void addRecord(const msg::MessageObject& record) override;
  void commit(TableChunkSummaryWriter* writer) override;

protected:
  double getValue(const msg::MessageObject& record) const;

  String summary_name_;
  uint32_t field_id_;
  msg::FieldType field_type_;
  size_t n_;
  NumericBoundsSummary summary_;
};

}
}
#endif

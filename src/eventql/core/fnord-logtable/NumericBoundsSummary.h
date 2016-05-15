/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _FNORD_LOGTABLE_NUMERICBOUNDSSUMMARYBUILDER_H
#define _FNORD_LOGTABLE_NUMERICBOUNDSSUMMARYBUILDER_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/util/binarymessagereader.h>
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

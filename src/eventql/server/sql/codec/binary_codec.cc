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
#include "eventql/server/sql/codec/binary_codec.h"
#include "eventql/sql/svalue.h"
#include "eventql/util/util/binarymessagereader.h"
#include "eventql/util/util/binarymessagewriter.h"

#include "eventql/eventql.h"

namespace csql {

BinaryResultParser::BinaryResultParser() :
    got_header_(false),
    got_footer_(false) {}

void BinaryResultParser::onTableHeader(
    Function<void (const Vector<String>& columns)> fn) {
  on_table_header_ = fn;
}

void BinaryResultParser::onRow(
    Function<void (int argc, const SValue* argv)> fn) {
  on_row_ = fn;
}

void BinaryResultParser::onProgress(
    Function<void (const ExecutionStatus& status)> fn) {
  on_progress_ = fn;
}

void BinaryResultParser::onError(Function<void (const String& error)> fn) {
  on_error_ = fn;
}

void BinaryResultParser::parse(const char* data, size_t size) {
  buf_.append(data, size);

  auto end = buf_.size();
  size_t cur = 0;
  bool eof = false;
  while (!eof && cur < end) {
    auto evtype = *buf_.structAt<uint8_t>(cur);

    switch (evtype) {

      // header
      case 0x01:
        ++cur;
        got_header_ = true;
        break;

      case 0xf1: {
        size_t res = parseTableHeader(buf_.structAt<void>(cur), end - cur);
        if (res > 0) {
          cur += res;
        } else {
          eof = true;
        }
        break;
      }

      case 0xf2: {
        size_t res = parseRow(buf_.structAt<void>(cur), end - cur);
        if (res > 0) {
          cur += res;
        } else {
          eof = true;
        }
        break;
      }

      case 0xf3: {
        size_t res = parseProgress(buf_.structAt<void>(cur), end - cur);
        if (res > 0) {
          cur += res;
        } else {
          eof = true;
        }
        break;
      }

      case 0xf4: {
        size_t res = parseError(buf_.structAt<void>(cur), end - cur);
        if (res > 0) {
          cur += res;
        } else {
          eof = true;
        }
        break;
      }

      // footer
      case 0xff:
        ++cur;
        got_footer_ = true;
        break;

      default:
        RAISEF(kParseError, "invalid event type: $0", evtype);

    }
  }

  if (cur > 0) {
    auto new_size = buf_.size() - cur;
    memmove(buf_.data(), (char*) buf_.data() + cur, new_size);
    buf_.resize(new_size);
  }
}

size_t BinaryResultParser::parseTableHeader(const void* data, size_t size) {
  util::BinaryMessageReader reader(data, size);

  uint8_t type;
  if (!reader.maybeReadUInt8(&type)) {
    return 0;
  }

  uint64_t ncols;
  if (!reader.maybeReadVarUInt(&ncols)) {
    return 0;
  }

  Vector<String> columns;
  for (uint64_t i = 0; i < ncols; ++i) {
    String colname;
    if (!reader.maybeReadLenencString(&colname)) {
      return 0;
    }

    columns.emplace_back(colname);
  }

  if (on_table_header_) {
    on_table_header_(columns);
  }

  return reader.position();
}

size_t BinaryResultParser::parseRow(const void* data, size_t size) {
  util::BinaryMessageReader reader(data, size);

  uint8_t type;
  if (!reader.maybeReadUInt8(&type)) {
    return 0;
  }

  uint64_t ncols;
  if (!reader.maybeReadVarUInt(&ncols)) {
    return 0;
  }

  Vector<SValue> row;
  for (uint64_t i = 0; i < ncols; ++i) {
    uint8_t stype;
    if (!reader.maybeReadUInt8(&stype)) {
      return 0;
    }

    switch (stype) {
      case SQL_STRING: {
        String val;
        if (reader.maybeReadLenencString(&val)) {
          row.emplace_back(SValue(val));
          break;
        } else {
          return 0;
        }
      }

      case SQL_FLOAT: {
        double val;
        if (reader.maybeReadDouble(&val)) {
          row.emplace_back(SValue(SValue::FloatType(val)));
          break;
        } else {
          return 0;
        }
      }

      case SQL_INTEGER: {
        uint64_t val;
        if (reader.maybeReadUInt64(&val)) {
          row.emplace_back(SValue(SValue::IntegerType(val)));
          break;
        } else {
          return 0;
        }
      }

      case SQL_BOOL: {
        uint8_t val;
        if (reader.maybeReadUInt8(&val)) {
          row.emplace_back(SValue(SValue::BoolType(val == 1)));
          break;
        } else {
          return 0;
        }
      }

      case SQL_TIMESTAMP: {
        uint64_t val;
        if (reader.maybeReadUInt64(&val)) {
          row.emplace_back(SValue(SValue::TimeType(val)));
          break;
        } else {
          return 0;
        }
      }

      case SQL_NULL: {
        row.emplace_back(SValue());
        break;
      }

    }
  }

  if (on_row_) {
    on_row_(row.size(), row.data());
  }

  return reader.position();
}

size_t BinaryResultParser::parseProgress(const void* data, size_t size) {
  util::BinaryMessageReader reader(data, size);

  uint8_t type;
  if (!reader.maybeReadUInt8(&type)) {
    return 0;
  }

  double progress;
  if (!reader.maybeReadDouble(&progress)) {
    return 0;
  }

  return reader.position();
}

size_t BinaryResultParser::parseError(const void* data, size_t size) {
  util::BinaryMessageReader reader(data, size);

  uint8_t type;
  if (!reader.maybeReadUInt8(&type)) {
    return 0;
  }

  String error_str;
  if (!reader.maybeReadLenencString(&error_str)) {
    return 0;
  }

  if (on_error_) {
    on_error_(error_str);
  }

  return reader.position();
}

bool BinaryResultParser::eof() const {
  return got_footer_;
}


BinaryResultFormat::BinaryResultFormat(
    WriteCallback write_cb,
    bool truncate_rows /* = false */) :
    write_cb_(write_cb),
    truncate_rows_(truncate_rows) {
  sendHeader();
}

BinaryResultFormat::~BinaryResultFormat() {
  sendFooter();
}

void BinaryResultFormat::sendProgress(double progress) {
  util::BinaryMessageWriter writer;
  writer.appendUInt8(0xf3);
  writer.appendDouble(progress);
  write_cb_(writer.data(), writer.size());
}

void BinaryResultFormat::sendError(const String& error) {
  util::BinaryMessageWriter writer;
  writer.appendUInt8(0xf4);
  writer.appendLenencString(error);
  write_cb_(writer.data(), writer.size());
}

void BinaryResultFormat::sendHeader() {
  util::BinaryMessageWriter writer;
  writer.appendUInt8(0x01);
  write_cb_(writer.data(), writer.size());
}

void BinaryResultFormat::sendFooter() {
  util::BinaryMessageWriter writer;
  writer.appendUInt8(0xff);
  write_cb_(writer.data(), writer.size());
}

void BinaryResultFormat::sendResults(QueryPlan* query) {
  //context->onStatusChange([this] (const csql::ExecutionStatus& status) {
  //  sendProgress(status.progress());
  //});

  try {
    for (int i = 0; i < query->numStatements(); ++i) {
      sendTable(query, i);
    }
  } catch (const StandardException& e) {
    sendError(e.what());
    throw e;
  }
}

void BinaryResultFormat::sendTable(
    QueryPlan* qplan,
    size_t stmt_idx) {
  auto result_columns = qplan->getStatementOutputColumns(stmt_idx);
  auto result_cursor = qplan->execute(stmt_idx);

  // table header
  {
    util::BinaryMessageWriter writer;
    writer.appendUInt8(0xf1);

    writer.appendVarUInt(result_columns.size());
    for (const auto& col : result_columns) {
      writer.appendLenencString(col);
    }

    write_cb_(writer.data(), writer.size());
  }

  auto num_cols = result_cursor->getNumColumns();
  if (truncate_rows_) {
    num_cols = result_columns.size();
  }

  Vector<SValue> row(num_cols);
  while (result_cursor->next(row.data(), row.size())) {
    Buffer buf;
    BufferOutputStream writer(&buf);
    writer.appendUInt8(0xf2);
    writer.appendVarUInt(row.size());
    for (const auto& val: row) {
      val.encode(&writer);
    }

    write_cb_(buf.data(), buf.size());
  }
}



}

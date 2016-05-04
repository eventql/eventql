/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "csql/runtime/BinaryResultParser.h"
#include "csql/svalue.h"
#include "stx/util/binarymessagereader.h"

using namespace stx;

namespace csql {

BinaryResultParser::BinaryResultParser() :
    got_header_(false),
    got_footer_(false) {}

void BinaryResultParser::onTableHeader(
    stx::Function<void (const Vector<String>& columns)> fn) {
  on_table_header_ = fn;
}

void BinaryResultParser::onRow(
    stx::Function<void (int argc, const SValue* argv)> fn) {
  on_row_ = fn;
}

void BinaryResultParser::onProgress(
    stx::Function<void (const ExecutionStatus& status)> fn) {
  on_progress_ = fn;
}

void BinaryResultParser::onError(stx::Function<void (const String& error)> fn) {
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

}

/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_BINARYFORMAT_H
#define _FNORD_CSTABLE_BINARYFORMAT_H
#include <stdlib.h>
#include <stdint.h>

namespace stx {
namespace cstable {

/**
 * // http://tools.ietf.org/html/rfc5234
 *
 *   <sstable> :=
 *       <header>
 *       <body>
 *       *<footer>
 *
 *   <cstable> :=
 *       <header>
 *       <body>
 *
 *   <header> :=
 *       %x17 %x23 %x17 %x23     // magic bytes
 *       %x00 %x01               // cstable file format version
 *       <uint64_t>              // flags
 *       <uint64_t>              // number of rows in the table
 *       <uint32_t>              // number of columns
 *       <column_header>*        // column header for each column
 *
 *   <column_header> :=
 *       <uint32_t>              // column type
 *       <uint32_t>              // length of the column name
 *       <char>*                 // column name
 *       <uint32_t>              // max repetition level
 *       <uint32_t>              // max definition level
 *       <uint64_t>              // column data start offset
 *       <uint64_t>              // column data size
 *
 */
class BinaryFormat {
public:
  static const uint16_t kVersion = 1;
  static const uint32_t kMagicBytes = 0x17231723;
};


enum class ColumnType : uint8_t {
  BOOLEAN = 1,
  UINT32_BITPACKED = 10,
  UINT32_PLAIN = 11,
  UINT64_PLAIN = 12,
  UINT64_LEB128 = 13,
  DOUBLE = 14,
  STRING_PLAIN = 100,
};

}
}

#endif

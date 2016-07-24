/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#ifndef _FNORD_SSTABLE_SSTABLE_H
#define _FNORD_SSTABLE_SSTABLE_H
#include <stdlib.h>
#include <stdint.h>


namespace sstable {

/**
 * // http://tools.ietf.org/html/rfc5234
 *
 *   <sstable> :=
 *       <header>
 *       <body>
 *       *<footer>
 *
 *   <header v1> :=
 *       %x17 %x17 %x17 %x17"    // magic bytes
 *       %x00 %x01               // sstable file format version
 *       <uint64_t>              // total body size in bytes
 *       <uint32_t>              // userdata checksum
 *       <uint32_t>              // userdata size in bytes
 *       <bytes>                 // userdata
 *
 *   <header v2> :=
 *       %x17 %x17 %x17 %x17"    // magic bytes
 *       %x00 %x02               // sstable file format version
 *       <uint64_t>              // flags (1=finalized)
 *       <uint64_t>              // total body size in bytes
 *       <uint32_t>              // userdata checksum
 *       <uint32_t>              // userdata size in bytes
 *       <bytes>                 // userdata
 *
 *   <header v3> :=
 *       %x17 %x17 %x17 %x17"    // magic bytes
 *       %x00 %x03               // sstable file format version
 *       <uint64_t>              // flags (1=finalized)
 *       <uint64_t>              // number of rows in the table
 *       <uint64_t>              // total body size in bytes
 *       <uint32_t>              // userdata checksum
 *       <uint32_t>              // userdata size in bytes
 *       <bytes>                 // userdata
 *
 *   <body> :=
 *       *<row>
 *
 *   <row> :=
 *       <uint32_t>              // row checksum
 *       <uint32_t>              // key size in bytes
 *       <uint32_t>              // data size in bytes
 *       <bytes>                 // key
 *       <bytes>                 // data
 *
 *   <footer> :=
 *       %x17 %x17 %x17 %x17"    // magic bytes
 *       <uint32_t>              // footer type id
 *       <uint32_t>              // userdata size in bytes
 *       <bytes>                 // userdata
 *
 */
enum class FileHeaderFlags : uint64_t {
  FINALIZED = 1
};

class BinaryFormat {
public:
  static const uint16_t kVersion = 3;
  static const uint64_t kMagicBytes = 0x17171717;

  struct __attribute__((packed)) RowHeader {
    uint32_t checksum;
    uint32_t key_size;
    uint32_t data_size;
  };

  struct __attribute__((packed)) FooterHeader {
    uint64_t magic;
    uint32_t type;
    uint32_t footer_checksum;
    uint32_t footer_size;
  };

};

}

#endif

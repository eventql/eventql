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
#include "eventql/eventql.h"
#include "eventql/util/stdtypes.h"
#include "eventql/util/random.h"
#include "eventql/db/metadata_store.h"
#include "../unit_test.h"
#include "../util/test_repository.h"

namespace eventql {
namespace test {
namespace unit {

// UNIT-METADATAFILE-001
static bool test_metadata_file_string_lookups() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "b";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "d";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "e";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile file(SHA1::compute("mytx"), 0, KEYSPACE_STRING, pmap, 0);

  EXPECT(file.hasFinitePartitions() == false);
  EXPECT(file.getPartitionMapAt("a") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapAt("b") == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapAt("bx") == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapAt("d") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapAt("dx") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapAt("e") == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapAt("ex") == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapAt("z") == file.getPartitionMapBegin() + 3);
  return true;
}

// UNIT-METADATAFILE-002
static bool test_metadata_file_user_defined_string_lookups() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "b";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "d";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "e";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile file(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_STRING,
      pmap,
      MFILE_USERDEFINED);

  EXPECT(file.hasFinitePartitions() == false);
  EXPECT(file.hasUserDefinedPartitions() == true);
  EXPECT(file.getPartitionMapAt("a") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt("b") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapAt("bx") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt("d") == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapAt("dx") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt("e") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapAt("ex") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt("z") == file.getPartitionMapEnd());
  return true;
}

// UNIT-METADATAFILE-003
static bool test_metadata_file_finite_uint_lookups() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "12");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "99");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile file(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  EXPECT(file.hasFinitePartitions() == true);
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "0")) == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "1")) == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "2")) == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "3")) == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "4")) == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "5")) == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "7")) == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "6")) == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "8")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "11")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "12")) == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "13")) == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt(encodePartitionKey(KEYSPACE_UINT64, "50")) == file.getPartitionMapBegin() + 3);
}

static String uint_encode(uint64_t v) {
  return String((const char*) &v, sizeof(v));
}

// UNIT-METADATAFILE-004
static bool test_metadata_file_uint_lookups() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = uint_encode(2);
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = uint_encode(5);
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = uint_encode(6);
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile file(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  EXPECT(file.hasFinitePartitions() == false);
  EXPECT(file.getPartitionMapAt(uint_encode(0)) == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapAt(uint_encode(1)) == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapAt(uint_encode(2)) == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapAt(uint_encode(3)) == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapAt(uint_encode(5)) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapAt(uint_encode(6)) == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapAt(uint_encode(7)) == file.getPartitionMapBegin() + 3);
  return true;
}

// UNIT-METADATAFILE-005
static bool test_metadata_file_range_lookups() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "b";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "d";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "e";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile file(SHA1::compute("mytx"), 0, KEYSPACE_STRING, pmap, 0);
  EXPECT(file.hasFinitePartitions() == false);
  EXPECT(file.getPartitionMapRangeBegin("") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin("a") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin("b") == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapRangeBegin("c") == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapRangeBegin("d") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeBegin("dx") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd("b") == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapRangeEnd("c") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd("d") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd("dx") == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapRangeEnd("f") == file.getPartitionMapBegin() + 4);
  EXPECT(file.getPartitionMapRangeEnd("z") == file.getPartitionMapBegin() + 4);
  EXPECT(file.getPartitionMapRangeEnd("") == file.getPartitionMapBegin() + 4);
  EXPECT(file.getPartitionMapRangeEnd("") == file.getPartitionMapEnd());
  return true;
}

// UNIT-METADATAFILE-006
static bool test_metadata_file_user_defined_range_lookups() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "a";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "b";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "d";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "e";
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile file(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_STRING,
      pmap,
      MFILE_USERDEFINED);

  EXPECT(file.hasFinitePartitions() == false);
  EXPECT(file.hasUserDefinedPartitions() == true);
  EXPECT(file.getPartitionMapRangeBegin("") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin("a") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin("b") == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapRangeBegin("c") == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapRangeBegin("d") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeBegin("dx") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd("b") == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapRangeEnd("c") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd("d") == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd("dx") == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapRangeEnd("f") == file.getPartitionMapBegin() + 4);
  EXPECT(file.getPartitionMapRangeEnd("z") == file.getPartitionMapBegin() + 4);
  EXPECT(file.getPartitionMapRangeEnd("") == file.getPartitionMapBegin() + 4);
  EXPECT(file.getPartitionMapRangeEnd("") == file.getPartitionMapEnd());
  return true;
}

// UNIT-METADATAFILE-007
static bool test_metadata_file_finite_uint_range_lookups() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "12");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "99");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile file(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  EXPECT(file.hasFinitePartitions() == true);
  EXPECT(file.getPartitionMapRangeBegin("") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "0")) == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "1")) == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "2")) == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "3")) == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "4")) == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "5")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "7")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "6")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "8")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "11")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "12")) == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "13")) == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "50")) == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapRangeBegin(encodePartitionKey(KEYSPACE_UINT64, "800")) == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapRangeEnd("") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "0")) == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "1")) == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "2")) == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "3")) == file.getPartitionMapBegin() + 1);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "4")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "5")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "7")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "6")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "8")) == file.getPartitionMapBegin() + 2);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "9")) == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "10")) == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "11")) == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "12")) == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "13")) == file.getPartitionMapBegin() +  3);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "14")) == file.getPartitionMapBegin() + 3);
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "50")) == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "99")) == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapRangeEnd(encodePartitionKey(KEYSPACE_UINT64, "800")) == file.getPartitionMapEnd());
}

// UNIT-METADATAFILE-008
static bool test_metadata_file_key_compare() {
  EXPECT(
      comparePartitionKeys(
          KEYSPACE_UINT64,
          encodePartitionKey(KEYSPACE_UINT64, "8"),
          encodePartitionKey(KEYSPACE_UINT64, "400")) == -1);

  EXPECT(
      comparePartitionKeys(
          KEYSPACE_UINT64,
          encodePartitionKey(KEYSPACE_UINT64, "400"),
          encodePartitionKey(KEYSPACE_UINT64, "8")) == 1);

  EXPECT(
      comparePartitionKeys(
          KEYSPACE_UINT64,
          encodePartitionKey(KEYSPACE_UINT64, "2"),
          encodePartitionKey(KEYSPACE_UINT64, "400")) == -1);

  return true;
}

// UNIT-METADATAFILE-009
static bool test_metadata_file_empty_range_lookups() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  MetadataFile file(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  EXPECT(file.hasFinitePartitions() == true);
  EXPECT(file.getPartitionMapAt("") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt("0") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt("10") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapAt("99") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapRangeBegin("") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin("0") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin("10") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeBegin("99") == file.getPartitionMapBegin());
  EXPECT(file.getPartitionMapRangeEnd("") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapRangeEnd("0") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapRangeEnd("10") == file.getPartitionMapEnd());
  EXPECT(file.getPartitionMapRangeEnd("99") == file.getPartitionMapEnd());
  return true;
}

void setup_unit_metadata_file_tests(TestRepository* repo) {
  std::vector<TestCase> c;
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATAFILE-001", metadata_file, string_lookups);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATAFILE-002", metadata_file, user_defined_string_lookups);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATAFILE-003", metadata_file, finite_uint_lookups);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATAFILE-004", metadata_file, uint_lookups);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATAFILE-005", metadata_file, range_lookups);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATAFILE-006", metadata_file, user_defined_range_lookups);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATAFILE-007", metadata_file, finite_uint_range_lookups);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATAFILE-008", metadata_file, key_compare);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATAFILE-009", metadata_file, empty_range_lookups);
  repo->addTestBundle(c);
}

} // namespace unit
} // namespace test
} // namespace eventql


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
#include <iostream>
#include "eventql/util/stdtypes.h"
#include "eventql/util/application.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/cli/CLI.h"
#include "eventql/util/csv/CSVInputStream.h"
#include "eventql/util/io/file.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/inspect.h"
#include "eventql/util/human.h"
#include "eventql/util/json/jsonoutputstream.h"
#include "eventql/util/protobuf/JSONEncoder.h"
#include "eventql/io/cstable/cstable_reader.h"
#include "eventql/io/cstable/RecordMaterializer.h"
#include "eventql/eventql.h"

static int cstable_dump_json(std::vector<std::string> args) {
  if (args.size() < 2) {
    std::cerr
        << "usage: cstable_tool dump-json <file.cst> <schema.json>"
        << std::endl;

    return 1;
  }

  msg::MessageSchema schema(nullptr);
  auto schema_json = json::parseJSON(FileUtil::read(args[1]).toString());
  schema.fromJSON(schema_json.begin(), schema_json.end());

  auto cstable_reader = cstable::CSTableReader::openFile(args[0]);
  cstable::RecordMaterializer cstable_materializer(
      &schema,
      cstable_reader.get());

  for (size_t i = 0; i < cstable_reader->numRecords(); ++i) {
    msg::MessageObject msg;
    cstable_materializer.nextRecord(&msg);

    std::string json_str;
    json::JSONOutputStream json(StringOutputStream::fromString(&json_str));
    msg::JSONEncoder::encode(msg, schema, &json);

    std::cout << json_str << std::endl;
  }

  return 0;
}

static int cstable_dump(std::vector<std::string> args) {
  if (args.size() < 1) {
    std::cerr << "usage: cstable_tool dump <file>" << std::endl;
    return 1;
  }

  String filename(args[0]);
  auto cstable = cstable::CSTableReader::openFile(filename);
  iputs("== GENERAL ==\n >> number of records: $0", cstable->numRecords());

  iputs("\n\n== INDEX ==", 1);
  for (const auto& c : cstable->columns()) {
    iputs(
        ">>  column_id=$0, column_name=$1",
        c.column_id,
        c.column_name);
  }

  auto page_mgr = cstable->getPageManager();
  for (const auto& e : page_mgr->getPageIndex()) {
    String type_str;
    switch (e.key.entry_type) {
      case cstable::PageIndexEntryType::DLEVEL:
        type_str = "DLVL";
        break;
      case cstable::PageIndexEntryType::RLEVEL:
        type_str = "RLVL";
        break;
      case cstable::PageIndexEntryType::DATA:
        type_str = "DATA";
        break;
    }

    iputs(
        ">>  column_id=$0 type=$1 offset=$2 size=$3",
        e.key.column_id,
        type_str,
        e.page.offset,
        e.page.size);
  }

  for (const auto& c : cstable->columns()) {
    iputs(
        "\n\n== COLUMN DATA for $0/$1 ==",
        c.column_id,
        c.column_name);

    auto col_reader = cstable->getColumnReader(c.column_name);
    uint64_t rlevel;
    uint64_t dlevel;
    String data;
    size_t i = 0;
    for (size_t j = 0; i < cstable->numRecords(); ++j) {
      col_reader->readString(&rlevel, &dlevel, &data);
      iputs(">>  idx=$0/$1 rlvl=$2 dlvl=$3 data=($4) '$5'", i + 1, j + 1, rlevel, dlevel, data.size(), data);
      if (col_reader->nextRepetitionLevel() == 0) {
        ++i;
      }
    }
  }

  return 0;
}

int main(int argc, const char** argv) {
  Application::init();
  signal(SIGPIPE, SIG_DFL);

  if (argc <= 1) {
    std::cerr << "usage: cstable_tool <cmd> ..." << std::endl;
    return 1;
  }

  std::string cmd(argv[1]);
  std::vector<std::string> args;
  for (int i = 2; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  if (cmd == "dump") {
    return cstable_dump(args);
  }

  if (cmd == "dump-json") {
    return cstable_dump_json(args);
  }

  std::cerr << "error: unknown command: " << cmd << std::endl;
  return 1;
}


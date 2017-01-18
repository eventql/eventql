/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <eventql/cli/commands/table_import.h>
#include <eventql/util/cli/flagparser.h>
#include "eventql/transport/native/frames/insert.h"
#include "eventql/transport/native/frames/error.h"

namespace eventql {
namespace cli {

enum class TableImportInputFormat { JSON, CSV };

const String TableImport::kName_ = "table-import";
const String TableImport::kDescription_ = "Import json or csv data to a table.";
static const char kEraseEscapeSequence[] = "\e[2K\r";

TableImport::TableImport(
    RefPtr<ProcessConfig> process_cfg) :
    process_cfg_(process_cfg),
    status_(ReturnCode::success()),
    complete_(false) {}

Status TableImport::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {
  ::cli::FlagParser flags;

  flags.defineFlag(
      "database",
      ::cli::FlagParser::T_STRING,
      true,
      "d",
      NULL,
      "database",
      "<string>");

  flags.defineFlag(
      "table",
      ::cli::FlagParser::T_STRING,
      true,
      "t",
      NULL,
      "table name",
      "<string>");

  flags.defineFlag(
      "host",
      ::cli::FlagParser::T_STRING,
      true,
      "h",
      NULL,
      "host name",
      "<string>");

  flags.defineFlag(
      "port",
      ::cli::FlagParser::T_INTEGER,
      true,
      "p",
      NULL,
      "port",
      "<integer>");

  flags.defineFlag(
      "file",
      ::cli::FlagParser::T_STRING,
      true,
      "f",
      NULL,
      "file path",
      "<string>");

  flags.defineFlag(
      "connections",
      ::cli::FlagParser::T_INTEGER,
      false,
      "c",
      "4",
      "number of connections",
      "<num>");

  flags.defineFlag(
      "format",
      ::cli::FlagParser::T_STRING,
      false,
      "F",
      "json",
      "input format (json or csv)",
      "<format>");

  try {
    flags.parseArgv(argv);
  } catch (const Exception& e) {
    stderr_os->write(StringUtil::format(
        "$0: $1\n",
        e.getTypeName(),
        e.getMessage()));
    return Status(e);
  }

  database_ = flags.getString("database");
  table_ = flags.getString("table");
  host_ = flags.getString("host");
  port_ = flags.getInt("port");
  num_threads_ = flags.isSet("connections") ?
      flags.getInt("connections") :
      kDefaultNumThreads;
  threads_.resize(num_threads_);
  is_tty_ = ::isatty(STDERR_FILENO);
  timeout_ =  process_cfg_->getInt("client.timeout").get();

  /* format */
  auto format_str = flags.getString("format");
  if (format_str == "json") {
    format_ = EVQL_INSERT_CTYPE_JSON;
  } else if (format_str == "csv") {
    format_ = EVQL_INSERT_CTYPE_CSV;
  } else {
    return Status(eRuntimeError, "invalid format");
  }

  /* auth data */
  if (process_cfg_->hasProperty("client.user")) {
    auth_data_.emplace_back(
        "user",
        process_cfg_->getString("client.user").get());
  }

  if (process_cfg_->hasProperty("client.password")) {
    auth_data_.emplace_back(
        "password",
        process_cfg_->getString("client.").get());
  }

  if (process_cfg_->hasProperty("client.auth_token")) {
    auth_data_.emplace_back(
        "auth_token",
        process_cfg_->getString("client.auth_token").get());
  }


  auto filename = flags.getString("file");
  if (filename == "-") {
    return run(stdin_is);
  } else {
    auto is = FileInputStream::openFile(filename);
    return run(is.get());
  }
}

Status TableImport::run(InputStream* is) {
  /* start threads */
  for (size_t i = 0; i < num_threads_; ++i) {
    threads_[i] = std::thread(std::bind(&TableImport::runThread, this));
  }

  /* if the input is a csv, read and store the header line */
  if (format_ == EVQL_INSERT_CTYPE_CSV) {
    if (!is->readLine(&csv_header_)) {
      return Status(eRuntimeError, "csv needs a header");
    }

    StringUtil::chomp(&csv_header_);
  }

  /* read and enqueue lines in batches */
  std::string line;
  std::vector<std::string> batch;
  while (is->readLine(&line)) {
    if (batch.size() > kDefaultBatchSize) {
      if (!enqueueBatch(std::move(batch))) {
        goto exit;
      }

      batch.clear();
    }

    batch.emplace_back(line);
    line.clear();

    // FIXME rate limiting
    printStats();
  }

  if (batch.size() > 0) {
    if (!enqueueBatch(std::move(batch))) {
      goto exit;
    }
  }

  /* send upload complete signal to trigger thread shutdown */
  {
    std::unique_lock<std::mutex> lk(mutex_);
    complete_ = true;
    cv_.notify_all();
  }

  printStats();

exit:

  /* join threads */
  for (auto& t : threads_) {
    if (t.joinable()) {
      t.join();
    }
  }

  if (is_tty_) {
    std::cerr << kEraseEscapeSequence << std::flush;
  }

  if (status_.isSuccess()) {
    std::cerr
        << "Successfully imported "
        << stats_.getTotalRowCount()
        << " row(s)"
        << std::endl;
  }

  return status_;
}

void TableImport::runThread() {
  try {
    native_transport::TCPClient client(
        nullptr,
        nullptr,
        timeout_,
        timeout_);

    auto rc = client.connect(
        host_,
        port_,
        false,
        auth_data_);

    if (!rc.isSuccess()) {
      setError(rc);
      return;
    }

    UploadBatch batch;
    while (popBatch(&batch)) {
      auto rc = uploadBatch(&client, batch);
      if (!rc.isSuccess()) {
        setError(rc);
        client.close();
        return;
      }

      stats_.addInsert(batch.size());
      batch.clear();
    }

    client.close();

  } catch (const std::exception& e) {
    setError(ReturnCode::exception(e));
  }
}

static const size_t kMaxQueueSize = 32;

bool TableImport::enqueueBatch(UploadBatch&& batch) {
  std::unique_lock<std::mutex> lk(mutex_);

  for (;;) {
    if (!status_.isSuccess() || complete_) {
      return false;
    }

    if (queue_.size() < kMaxQueueSize) {
      break;
    }

    cv_.wait(lk);
  }


  queue_.push_back(std::move(batch));
  cv_.notify_all();
  return true;
}

bool TableImport::popBatch(UploadBatch* batch) {
  std::unique_lock<std::mutex> lk(mutex_);

  for (;;) {
    if (!status_.isSuccess()) {
      return false;
    }

    if (!queue_.empty()) {
      break;
    }

    if (complete_) {
      return false;
    } else {
      cv_.wait(lk);
    }
  }


  *batch = queue_.front();
  queue_.pop_front();
  cv_.notify_all();
  return true;
}

void TableImport::setError(const ReturnCode& err) {
  std::unique_lock<std::mutex> lk(mutex_);
  status_ = err;
  cv_.notify_all();
}

Status TableImport::uploadBatch(
    native_transport::TCPClient* client,
    const UploadBatch& batch) {
  /* insert */
  native_transport::InsertFrame i_frame;
  i_frame.setDatabase(database_);
  i_frame.setTable(table_);
  i_frame.setRecordEncoding(format_);

  if (format_ == EVQL_INSERT_CTYPE_CSV) {
    i_frame.setRecordEncodingInfo(csv_header_);
  }

  for (auto l : batch) {
    auto line = l;
    StringUtil::chomp(&line);
    i_frame.addRecord(line);
  }

  size_t retry_timeout = kMicrosPerSecond;
  size_t retry_timeout_max = kMicrosPerSecond * 30;
  for (size_t nretry = 0; nretry < 100; ++nretry) {
    if (nretry > 0) {
      usleep(retry_timeout);
      retry_timeout = std::min(retry_timeout * 2, retry_timeout_max);
    }

    if (!client->isConnected()) {
      auto rc = client->connect(
          host_,
          port_,
          false,
          auth_data_);

      if (!rc.isSuccess()) {
        printError(rc.getMessage());
        continue;
      }
    }

    auto rc = client->sendFrame(&i_frame, 0);
    if (!rc.isSuccess()) {
      printError(rc.getMessage());
      continue;
    }

    uint16_t ret_opcode = 0;
    uint16_t ret_flags;
    std::string ret_payload;
    bool success = true;
    while (success && ret_opcode != EVQL_OP_ACK) {
      auto rc = client->recvFrame(
          &ret_opcode,
          &ret_flags,
          &ret_payload,
          kMicrosPerSecond); // FIXME

      if (!rc.isSuccess()) {
        printError(rc.getMessage());
        success = false;
        break;
      }

      switch (ret_opcode) {
        case EVQL_OP_ACK:
        case EVQL_OP_HEARTBEAT:
          continue;
        case EVQL_OP_ERROR: {
          native_transport::ErrorFrame eframe;
          eframe.parseFrom(ret_payload.data(), ret_payload.size());
          if (!eframe.isRetryable()) {
            return ReturnCode::error("ERUNTIME", eframe.getError());
          }

          printError(eframe.getError());
          success = false;
          break;
        }
        default:
          return ReturnCode::error("ERUNTIME", "unexpected opcode");
      }
    }

    if (success) {
      return Status::success();
    } else {
      continue;
    }
  }

  return ReturnCode::error("ERUNTIME", "retry limit reacehd");
}

void TableImport::printStats() {
  std::stringstream line;

  if (is_tty_) {
    line << kEraseEscapeSequence;
  }

  line << std::fixed
      << "Inserting... "
      << "rate="
      << std::setprecision(2) << stats_.getRollingRPS()
      << "rows/s"
      << ", total="
      << stats_.getTotalRowCount();

  if (!is_tty_) {
    line << std::endl;
  }

  std::cerr << line.str() << std::flush;
}

void TableImport::printError(const std::string& error) {
  std::cerr << kEraseEscapeSequence << "ERROR: " << error << std::endl;
}

const String& TableImport::getName() const {
  return kName_;
}

const String& TableImport::getDescription() const {
  return kDescription_;
}

void TableImport::printHelp(OutputStream* stdout_os) const {
  stdout_os->write(StringUtil::format(
      "\nevqlctl-$0 - $1\n\n", kName_, kDescription_));

  stdout_os->write(
      "Usage: evqlctl table-import [OPTIONS]\n"
      "   -c, --connections <num>     Number of concurrent connections\n"
      "  -d, --database <db>          Select a database.\n"
      "  -t, --table <tbl>            Select a destination table.\n"
      "  -f, --file <file>            Set the path of the file to import.\n"
      "  -h, --host <hostname>        Set the EventQL hostname.\n"
      "  -p, --port <port>            Set the EventQL port.\n");
}

TableImportStats::TableImportStats() : total_row_count_(0) {}

void TableImportStats::addInsert(uint64_t num_rows) {
  std::unique_lock<std::mutex> lk(mutex_);
  rolling_rps_.addValue(num_rows);
  total_row_count_ += num_rows;
}

double TableImportStats::getRollingRPS() const {
  std::unique_lock<std::mutex> lk(mutex_);

  uint64_t value;
  uint64_t count;
  uint64_t interval_us;
  rolling_rps_.computeAggregate(&value, &count, &interval_us);

  return double(value) / (double(interval_us) / kMicrosPerSecond);
}

uint64_t TableImportStats::getTotalRowCount() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return total_row_count_;
}

} // namespace cli
} // namespace eventql


/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/BinaryResultFormat.h>
#include <stx/logging.h>

namespace csql {

BinaryResultFormat::BinaryResultFormat(
    WriteCallback write_cb) :
    write_cb_(write_cb) {
  sendHeader();
}

BinaryResultFormat::~BinaryResultFormat() {
  sendFooter();
}

void BinaryResultFormat::sendProgress(double progress) {
  stx::util::BinaryMessageWriter writer;
  writer.appendUInt8(0xf3);
  writer.appendDouble(progress);
  write_cb_(writer.data(), writer.size());
}

void BinaryResultFormat::sendError(const String& error) {
  stx::util::BinaryMessageWriter writer;
  writer.appendUInt8(0xf4);
  writer.appendLenencString(error);
  write_cb_(writer.data(), writer.size());
}

void BinaryResultFormat::sendHeader() {
  stx::util::BinaryMessageWriter writer;
  writer.appendUInt8(0x01);
  write_cb_(writer.data(), writer.size());
}

void BinaryResultFormat::sendFooter() {
  stx::util::BinaryMessageWriter writer;
  writer.appendUInt8(0xff);
  write_cb_(writer.data(), writer.size());
}

void BinaryResultFormat::formatResults(
    RefPtr<QueryPlan> query,
    ExecutionContext* context) {
  context->onStatusChange([this] (const csql::ExecutionStatus& status) {
    sendProgress(status.progress());
  });

  try {
    for (int i = 0; i < query->numStatements(); ++i) {
      auto stmt = query->getStatement(i);

      auto table_expr = dynamic_cast<TableExpression*>(stmt);
      if (table_expr) {
        sendTable(table_expr, context);
        continue;
      }

      RAISE(kRuntimeError, "can't render DRAW statement in BinaryFormat")
    }
  } catch (const StandardException& e) {
    stx::logError("sql", e, "SQL execution failed");
    sendError(e.what());
  }
}

void BinaryResultFormat::sendTable(
    TableExpression* stmt,
    ExecutionContext* context) {

  // table header
  {
    stx::util::BinaryMessageWriter writer;
    writer.appendUInt8(0xf1);

    auto columns = stmt->columnNames();
    writer.appendVarUInt(columns.size());
    for (const auto& col : columns) {
      writer.appendLenencString(col);
    }

    write_cb_(writer.data(), writer.size());
  }

  stmt->execute(
      context,
      [this] (int argc, const csql::SValue* argv) -> bool {
    Buffer buf;
    BufferOutputStream writer(&buf);
    writer.appendUInt8(0xf2);
    writer.appendVarUInt(argc);
    for (int n = 0; n < argc; ++n) {
      argv[n].encode(&writer);
    }

    write_cb_(buf.data(), buf.size());
    return true;
  });
}

}

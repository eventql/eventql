/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/io/BufferedOutputStream.h>
#include <stx/io/fileutil.h>
#include <chartsql/runtime/groupby.h>

namespace csql {

GroupBy::GroupBy(
    ScopedPtr<TableExpression> source,
    const Vector<String>& column_names,
    Vector<ScopedPtr<ValueExpression>> select_expressions,
    Vector<ScopedPtr<ValueExpression>> group_expressions,
    SHA1Hash qtree_fingerprint) :
    source_(std::move(source)),
    column_names_(column_names),
    select_exprs_(std::move(select_expressions)),
    group_exprs_(std::move(group_expressions)),
    qtree_fingerprint_(qtree_fingerprint) {}

void GroupBy::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  HashMap<String, Vector<VM::Instance >> groups;
  ScratchMemory scratch;

  try {
    accumulate(&groups, &scratch, context);
    getResult(&groups, fn);
  } catch (...) {
    freeResult(&groups);
    throw;
  }

  freeResult(&groups);
}

void GroupBy::accumulate(
    HashMap<String, Vector<VM::Instance >>* groups,
    ScratchMemory* scratch,
    ExecutionContext* context) {
  auto cache_key = cacheKey();
  String cache_filename;
  bool from_cache = false;
  auto cachedir = context->cacheDir();
  if (!cache_key.isEmpty() && !cachedir.isEmpty()) {
    cache_filename = FileUtil::joinPaths(
        cachedir.get(),
        cache_key.get().toString() + ".qcache");

    if (FileUtil::exists(cache_filename)) {
      auto fis = FileInputStream::openFile(cache_filename);
      from_cache = decode(groups, scratch, fis.get());
    }
  }

  if (!from_cache) {
    source_->execute(
        context,
        std::bind(
            &GroupBy::nextRow,
            this,
            groups,
            scratch,
            std::placeholders::_1,
            std::placeholders::_2));
  }

  if (!from_cache && !cache_key.isEmpty() && !cachedir.isEmpty()) {
    BufferedOutputStream fos(
        FileOutputStream::fromFile(
            File::openFile(
                cache_filename + "~",
                File::O_CREATEOROPEN | File::O_WRITE | File::O_TRUNCATE)));

    encode(groups, &fos);
    FileUtil::mv(cache_filename + "~", cache_filename);
  }
}

void GroupBy::getResult(
    const HashMap<String, Vector<VM::Instance >>* groups,
    Function<bool (int argc, const SValue* argv)> fn) {
  Vector<SValue> out_row(select_exprs_.size(), SValue{});
  for (auto& group : *groups) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      select_exprs_[i]->result(&group.second[i], &out_row[i]);
    }

    if (!fn(out_row.size(), out_row.data())) {
      return;
    }
  }
}

void GroupBy::freeResult(
    HashMap<String, Vector<VM::Instance >>* groups) {
  for (auto& group : (*groups)) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      select_exprs_[i]->freeInstance(&group.second[i]);
    }
  }
}

void GroupBy::mergeResult(
    const HashMap<String, Vector<VM::Instance >>* src,
    HashMap<String, Vector<VM::Instance >>* dst,
    ScratchMemory* scratch) {
  for (const auto& src_group : *src) {
    auto& dst_group = (*dst)[src_group.first];
    if (dst_group.size() == 0) {
      for (const auto& e : select_exprs_) {
        dst_group.emplace_back(e->allocInstance(scratch));
      }
    }

    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      select_exprs_[i]->merge(&dst_group[i], &src_group.second[i]);
    }
  }
}

bool GroupBy::nextRow(
    HashMap<String, Vector<VM::Instance >>* groups,
    ScratchMemory* scratch,
    int row_len, const SValue* row) {
  Vector<SValue> gkey(group_exprs_.size(), SValue{});
  for (size_t i = 0; i < group_exprs_.size(); ++i) {
    group_exprs_[i]->evaluate(row_len, row, &gkey[i]);
  }

  auto group_key = SValue::makeUniqueKey(gkey.data(), gkey.size());
  auto& group = (*groups)[group_key];
  if (group.size() == 0) {
    for (const auto& e : select_exprs_) {
      group.emplace_back(e->allocInstance(scratch));
    }
  }

  for (size_t i = 0; i < select_exprs_.size(); ++i) {
    select_exprs_[i]->accumulate(&group[i], row_len, row);
  }

  return true;
}

Vector<String> GroupBy::columnNames() const {
  return column_names_;
}

size_t GroupBy::numColumns() const {
  return column_names_.size();
}

void GroupBy::encode(
    const HashMap<String, Vector<VM::Instance >>* groups,
    OutputStream* os) const {
  os->appendVarUInt(groups->size());
  os->appendVarUInt(select_exprs_.size());

  for (auto& group : *groups) {
    os->appendLenencString(group.first);

    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      select_exprs_[i]->saveState(&group.second[i], os);
    }
  }
}

bool GroupBy::decode(
    HashMap<String, Vector<VM::Instance >>* groups,
    ScratchMemory* scratch,
    InputStream* is) const {
  auto ngroups = is->readVarUInt();
  auto nexprs = is->readVarUInt();

  if (select_exprs_.size() != nexprs) {
    return false;
  }

  for (size_t j = 0; j < ngroups; ++j) {
    auto group_key = is->readLenencString();

    auto& group = (*groups)[group_key];
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      const auto& e = select_exprs_[i];
      group.emplace_back(e->allocInstance(scratch));
      e->loadState(&group[i], is);
    }
  }

  return true;
}

Option<SHA1Hash> GroupBy::cacheKey() const {
  auto source_key = source_->cacheKey();
  if (source_key.isEmpty()) {
    return None<SHA1Hash>();
  }

  return Some(
      SHA1::compute(
          StringUtil::format(
              "$0~$1",
              source_key.get().toString(),
              qtree_fingerprint_.toString())));
}


}

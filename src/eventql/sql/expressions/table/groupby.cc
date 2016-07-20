/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include <eventql/util/io/BufferedOutputStream.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/logging.h>
#include <eventql/util/random.h>
#include <eventql/sql/expressions/table/groupby.h>
#include <eventql/util/freeondestroy.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace csql {

GroupByExpression::GroupByExpression(
    Transaction* txn,
    ExecutionContext* execution_context,
    Vector<ValueExpression> select_expressions,
    Vector<ValueExpression> group_expressions,
    ScopedPtr<TableExpression> input) :
    txn_(txn),
    execution_context_(execution_context),
    select_exprs_(std::move(select_expressions)),
    group_exprs_(std::move(group_expressions)),
    input_(std::move(input)),
    freed_(false) {
  execution_context_->incrementNumTasks();
}

GroupByExpression::~GroupByExpression() {
  if (!freed_) {
    freeResult();
  }
}

ScopedPtr<ResultCursor> GroupByExpression::execute() {
  execution_context_->incrementNumTasksRunning();

  auto input_cursor = input_->execute();
  Vector<SValue> row(input_cursor->getNumColumns());
  while (input_cursor->next(row.data(), row.size())) {
    Vector<SValue> gkey(group_exprs_.size(), SValue{});
    for (size_t i = 0; i < group_exprs_.size(); ++i) {
      VM::evaluate(
          txn_,
          group_exprs_[i].program(),
          row.size(),
          row.data(),
          &gkey[i]);
    }

    auto group_key = SValue::makeUniqueKey(gkey.data(), gkey.size());
    auto& group = groups_[group_key];
    if (group.size() == 0) {
      for (const auto& e : select_exprs_) {
        group.emplace_back(VM::allocInstance(txn_, e.program(), &scratch_));
      }
    }

    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::accumulate(
          txn_,
          select_exprs_[i].program(),
          &group[i],
          row.size(),
          row.data());
    }
  }

  groups_iter_ = groups_.begin();
  return mkScoped(
      new DefaultResultCursor(
          select_exprs_.size(),
          std::bind(
              &GroupByExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

size_t GroupByExpression::getNumColumns() const {
  return select_exprs_.size();
}

bool GroupByExpression::next(SValue* row, size_t row_len) {
  if (groups_iter_ != groups_.end()) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::result(txn_, select_exprs_[i].program(), &groups_iter_->second[i], &row[i]);
    }

    if (++groups_iter_ == groups_.end()) {
      execution_context_->incrementNumTasksCompleted();
      freeResult();
    }
    return true;
  } else {
    return false;
  }
}

void GroupByExpression::freeResult() {
  for (auto& group : groups_) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::freeInstance(txn_, select_exprs_[i].program(), &group.second[i]);
    }
  }

  groups_.clear();
  freed_ = true;
}

PartialGroupByExpression::PartialGroupByExpression(
    Transaction* txn,
    Vector<ValueExpression> select_expressions,
    Vector<ValueExpression> group_expressions,
    SHA1Hash expression_fingerprint,
    ScopedPtr<TableExpression> input) :
    txn_(txn),
    select_exprs_(std::move(select_expressions)),
    group_exprs_(std::move(group_expressions)),
    expression_fingerprint_(expression_fingerprint),
    input_(std::move(input)),
    freed_(false) {}

PartialGroupByExpression::~PartialGroupByExpression() {
  if (!freed_) {
    freeResult();
  }
}

ScopedPtr<ResultCursor> PartialGroupByExpression::execute() {
  bool from_cache = false;
  auto cache_key = getCacheKey();
  auto cache_dir = txn_->getCacheDirectory();

  // read cache
  if (!cache_dir.empty() && !cache_key.isEmpty()) {
    auto cache_file_path = FileUtil::joinPaths(
        cache_dir,
        cache_key.get().toString() + ".qc");

    int cache_file_fd = open(cache_file_path.c_str(), O_RDONLY);
    if (cache_file_fd >= 0) {
      auto cache_file_is = FileInputStream::fromFileDescriptor(
          cache_file_fd,
          true);

      cache_file_is->readUInt8();
      auto num_groups = cache_file_is->readUInt64();
      for (uint64_t i = 0; i < num_groups; ++i) {
        auto group_key_len = cache_file_is->readUInt32();
        Buffer group_key(group_key_len);
        cache_file_is->readNextBytes(group_key.data(), group_key_len);
        auto& group = groups_[group_key.toString()];

        if (group.size() == 0) {
          for (const auto& e : select_exprs_) {
            group.emplace_back(VM::allocInstance(txn_, e.program(), &scratch_));
          }
        }

        for (size_t i = 0; i < select_exprs_.size(); ++i) {
          VM::loadState(
              txn_,
              select_exprs_[i].program(),
              &group[i],
              cache_file_is.get());
        }
      }

      from_cache = true;
    }
  }

  // execute
  if (!from_cache) {
    auto input_cursor = input_->execute();
    Vector<SValue> row(input_cursor->getNumColumns());
    while (input_cursor->next(row.data(), row.size())) {
      Vector<SValue> gkey(group_exprs_.size(), SValue{});
      for (size_t i = 0; i < group_exprs_.size(); ++i) {
        VM::evaluate(
            txn_,
            group_exprs_[i].program(),
            row.size(),
            row.data(),
            &gkey[i]);
      }

      auto group_key = SValue::makeUniqueKey(gkey.data(), gkey.size());
      auto& group = groups_[group_key];
      if (group.size() == 0) {
        for (const auto& e : select_exprs_) {
          group.emplace_back(VM::allocInstance(txn_, e.program(), &scratch_));
        }
      }

      for (size_t i = 0; i < select_exprs_.size(); ++i) {
        VM::accumulate(
            txn_,
            select_exprs_[i].program(),
            &group[i],
            row.size(),
            row.data());
      }
    }
  }

  // store cache
  if (!cache_dir.empty() && !cache_key.isEmpty() && !from_cache) {
    auto cache_file_path = FileUtil::joinPaths(
        cache_dir,
        cache_key.get().toString() + ".qc");
    auto cache_file_path_tmp =
        cache_file_path + "." + Random::singleton()->hex64();

    auto cache_file_os = FileOutputStream::openFile(cache_file_path_tmp);

    cache_file_os->appendUInt8(0x01);
    cache_file_os->appendUInt64(groups_.size());
    for (const auto& g : groups_) {
      cache_file_os->appendUInt32(g.first.size());
      cache_file_os->write(g.first.data(), g.first.size());

      for (size_t i = 0; i < select_exprs_.size(); ++i) {
        VM::saveState(
            txn_,
            select_exprs_[i].program(),
            &g.second[i],
            cache_file_os.get());
      }
    }

    // ignore errors
    ::rename(cache_file_path_tmp.c_str(), cache_file_path.c_str());
  }

  groups_iter_ = groups_.begin();
  return mkScoped(
      new DefaultResultCursor(
          2,
          std::bind(
              &PartialGroupByExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

Option<SHA1Hash> PartialGroupByExpression::getCacheKey() const {
  auto input_cache_key = input_->getCacheKey();
  if (input_cache_key.isEmpty()) {
    return None<SHA1Hash>();
  } else {
    return SHA1::compute(
        input_cache_key.get().toString() + expression_fingerprint_.toString());
  }
}

size_t PartialGroupByExpression::getNumColumns() const {
  return 2;
}

bool PartialGroupByExpression::next(SValue* row, size_t row_len) {
  if (groups_iter_ != groups_.end()) {
    String group_data;
    auto group_data_os = StringOutputStream::fromString(&group_data);
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::saveState(
          txn_,
          select_exprs_[i].program(),
          &groups_iter_->second[i],
          group_data_os.get());
    }

    switch (row_len) {
      case 2:
        row[1] = SValue::newString(group_data);
      case 1:
        row[0] = SValue::newString(groups_iter_->first);
      default:
        break;
    }

    if (++groups_iter_ == groups_.end()) {
      freeResult();
    }
    return true;
  } else {
    return false;
  }
}


void PartialGroupByExpression::freeResult() {
  for (auto& group : groups_) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::freeInstance(txn_, select_exprs_[i].program(), &group.second[i]);
    }
  }

  groups_.clear();
  freed_ = true;
}

GroupByMergeExpression::GroupByMergeExpression(
    Transaction* txn,
    ExecutionContext* execution_context,
    Vector<ValueExpression> select_expressions,
    ScopedPtr<TableExpression> input) :
    txn_(txn),
    execution_context_(execution_context),
    select_exprs_(std::move(select_expressions)),
    input_(std::move(input)),
    freed_(false) {
  execution_context_->incrementNumTasks();
}

GroupByMergeExpression::~GroupByMergeExpression() {
  if (!freed_) {
    freeResult();
  }
}

ScopedPtr<ResultCursor> GroupByMergeExpression::execute() {
  execution_context_->incrementNumTasksRunning();
  auto input_cursor = input_->execute();
  Vector<SValue> row(2);

  ScratchMemory scratch;
  Vector<VM::Instance> remote_group;
  for (size_t i = 0; i < select_exprs_.size(); ++i) {
    const auto& e = select_exprs_[i];
    remote_group.emplace_back(VM::allocInstance(txn_, e.program(), &scratch));
  }

  RunOnDestroy remote_group_finalizer([this, &remote_group] {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::freeInstance(txn_, select_exprs_[i].program(), &remote_group[i]);
    }
  });

  while (input_cursor->next(row.data(), row.size())) {
    const auto& group_key = row[0].getString();

    auto& group = groups_[group_key];
    if (group.size() == 0) {
      for (const auto& e : select_exprs_) {
        group.emplace_back(VM::allocInstance(txn_, e.program(), &scratch_));
      }
    }

    auto is = StringInputStream::fromString(row[1].getString());
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      const auto& e = select_exprs_[i];
      VM::loadState(txn_, e.program(), &remote_group[i], is.get());
      VM::merge(txn_, e.program(), &group[i], &remote_group[i]);
    }
  }

  groups_iter_ = groups_.begin();
  return mkScoped(
      new DefaultResultCursor(
          select_exprs_.size(),
          std::bind(
              &GroupByMergeExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

size_t GroupByMergeExpression::getNumColumns() const {
  return select_exprs_.size();
}

bool GroupByMergeExpression::next(SValue* row, size_t row_len) {
  if (groups_iter_ != groups_.end()) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::result(txn_, select_exprs_[i].program(), &groups_iter_->second[i], &row[i]);
    }

    if (++groups_iter_ == groups_.end()) {
      execution_context_->incrementNumTasksCompleted();
      freeResult();
    }
    return true;
  } else {
    return false;
  }
}

void GroupByMergeExpression::freeResult() {
  for (auto& group : groups_) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::freeInstance(txn_, select_exprs_[i].program(), &group.second[i]);
    }
  }

  groups_.clear();
  freed_ = true;
}

} // namespace csql

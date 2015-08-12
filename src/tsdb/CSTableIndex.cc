/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stdtypes.h>
#include <stx/logging.h>
#include <stx/io/fileutil.h>
#include <stx/io/mmappedfile.h>
#include <stx/protobuf/msg.h>
#include <stx/wallclock.h>
#include <tsdb/CSTableIndex.h>
#include <tsdb/CSTableIndexBuildState.pb.h>
#include <tsdb/RecordSet.h>
#include <stx/protobuf/MessageDecoder.h>
#include <cstable/CSTableBuilder.h>

using namespace stx;

namespace tsdb {

CSTableIndex::CSTableIndex(PartitionMap* pmap) :
    queue_([] (
        const Pair<uint64_t, RefPtr<Partition>>& a,
        const Pair<uint64_t, RefPtr<Partition>>& b) {
      return a.first < b.first;
    }),
    running_(false) {
  pmap->subscribeToPartitionChanges([this] (
      RefPtr<tsdb::PartitionChangeNotification> change) {
    enqueuePartition(change->partition);
  });

  start();
}

CSTableIndex::~CSTableIndex() {
  stop();
}

Option<RefPtr<VFSFile>> CSTableIndex::fetchCSTable(
    const String& tsdb_namespace,
    const String& table,
    const SHA1Hash& partition) const {
  auto filepath = fetchCSTableFilename(tsdb_namespace, table, partition);

  if (filepath.isEmpty()) {
    return None<RefPtr<VFSFile>>();
  } else {
    return Some<RefPtr<VFSFile>>(
        new io::MmappedFile(File::openFile(filepath.get(), File::O_READ)));
  }
}

Option<String> CSTableIndex::fetchCSTableFilename(
    const String& tsdb_namespace,
    const String& table,
    const SHA1Hash& partition) const {
  RAISE(kNotImplementedError);

  //auto filepath = FileUtil::joinPaths(
  //    db_path_,
  //    StringUtil::format(
  //        "$0/$1/$2/_cstable",
  //        tsdb_namespace,
  //        SHA1::compute(table).toString(),
  //        partition.toString()));

  //if (FileUtil::exists(filepath)) {
  //  return Some(filepath);
  //} else {
  //  return None<String>();
  //}
}

bool CSTableIndex::needsUpdate(
    RefPtr<PartitionSnapshot> snap) const {
  String tbl_uuid((char*) snap->uuid().data(), snap->uuid().size());

  auto metapath = FileUtil::joinPaths(snap->base_path, "_cstable_state");
  //auto metapath_tmp = metapath + "." + Random::singleton()->hex128();

  if (FileUtil::exists(metapath)) {
    auto metadata = msg::decode<CSTableIndexBuildState>(
        FileUtil::read(metapath));

    if (metadata.uuid() == tbl_uuid &&
        metadata.offset() >= snap->nrecs) {
      return false;
    }
  }

  return true;
}

void CSTableIndex::buildCSTable(RefPtr<Partition> partition) {
  auto table = partition->getTable();
  if (table->storage() == tsdb::TBL_STORAGE_STATIC) {
    return;
  }

  auto t0 = WallClock::unixMicros();
  auto snap = partition->getSnapshot();

  if (!needsUpdate(snap)) {
    return;
  }

  logDebug(
      "tsdb",
      "Building CSTable index for partition $0/$1/$2",
      snap->state.tsdb_namespace(),
      table->name(),
      snap->key.toString());

  auto filepath = FileUtil::joinPaths(snap->base_path, "_cstable");
  auto filepath_tmp = filepath + "." + Random::singleton()->hex128();
  auto schema = table->schema();

  {
    cstable::CSTableBuilder cstable(schema.get());

    auto reader = partition->getReader();
    reader->fetchRecords([&schema, &cstable] (const Buffer& record) {
        msg::MessageObject obj;
        msg::MessageDecoder::decode(record.data(), record.size(), *schema, &obj);
        cstable.addRecord(obj);
    });

    cstable.write(filepath_tmp);
  }

  auto metapath = FileUtil::joinPaths(snap->base_path, "_cstable_state");
  auto metapath_tmp = metapath + "." + Random::singleton()->hex128();
  {
    auto metafile = File::openFile(
        metapath_tmp,
        File::O_CREATE | File::O_WRITE);

    CSTableIndexBuildState metadata;
    metadata.set_offset(snap->nrecs);
    metadata.set_uuid(snap->uuid().data(), snap->uuid().size());

    auto buf = msg::encode(metadata);
    metafile.write(buf->data(), buf->size());
  }

  FileUtil::mv(filepath_tmp, filepath);
  FileUtil::mv(metapath_tmp, metapath);

  auto t1 = WallClock::unixMicros();
  logDebug(
      "tsdb",
      "Commiting CSTable index for partition $0/$1/$2, building took $3s",
      snap->state.tsdb_namespace(),
      table->name(),
      snap->key.toString(),
      (double) (t1 - t0) / 1000000.0f);
}

void CSTableIndex::enqueuePartition(RefPtr<Partition> partition) {
  auto interval = partition->getTable()->cstableBuildInterval();

  std::unique_lock<std::mutex> lk(mutex_);

  auto uuid = partition->uuid();
  if (waitset_.count(uuid) > 0) {
    return;
  }

  queue_.emplace(
      WallClock::unixMicros() + interval.microseconds(),
      partition);

  waitset_.emplace(uuid);
  cv_.notify_all();
}

void CSTableIndex::start() {
  running_ = true;

  for (int i = 0; i < 4; ++i) {
    threads_.emplace_back(std::bind(&CSTableIndex::work, this));
  }
}

void CSTableIndex::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  cv_.notify_all();

  for (auto& t : threads_) {
    t.join();
  }
}

void CSTableIndex::work() {
  std::unique_lock<std::mutex> lk(mutex_);

  while (running_) {
    if (queue_.size() == 0) {
      cv_.wait(lk);
    }

    if (queue_.size() == 0) {
      continue;
    }

    auto now = WallClock::unixMicros();
    if (now < queue_.begin()->first) {
      cv_.wait_for(
          lk,
          std::chrono::microseconds(queue_.begin()->first - now));

      continue;
    }

    auto partition = queue_.begin()->second;
    queue_.erase(queue_.begin());

    bool success = true;
    {
      lk.unlock();

      try {
        buildCSTable(partition);
      } catch (const StandardException& e) {
        logError("tsdb", e, "CSTableIndex error");
        success = false;
      }

      lk.lock();
    }

    if (success) {
      waitset_.erase(partition->uuid());

      if (needsUpdate(partition->getSnapshot())) {
        enqueuePartition(partition);
      }
    } else {
      auto delay = 30 * kMicrosPerSecond; // FIXPAUL increasing delay..
      queue_.emplace(now + delay, partition);
    }
  }
}

} // namespace tsdb

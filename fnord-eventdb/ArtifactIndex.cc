/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/exception.h>
#include <fnord-base/logging.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-base/io/FileLock.h>
#include <fnord-base/io/fileutil.h>
#include <fnord-eventdb/ArtifactIndex.h>

namespace fnord {
namespace eventdb {

ArtifactIndex::ArtifactIndex(
    const String& db_path,
    const String& index_name,
    bool readonly) :
    db_path_(db_path),
    index_name_(index_name),
    readonly_(readonly),
    exists_(false),
    index_file_(StringUtil::format("$0/$1.idx", db_path_, index_name_)),
    index_lockfile_(StringUtil::format("$0/$1.lck", db_path_, index_name_)),
    cached_mtime_(0) {}

List<ArtifactRef> ArtifactIndex::listArtifacts() {
  return readIndex();
}

void ArtifactIndex::addArtifact(const ArtifactRef& artifact) {
  fnord::logDebug("fn.evdb", "Adding artifact: $0", artifact.name);

  withIndex(false, [&] (List<ArtifactRef>* index) {
    for (const auto& a : *index) {
      if (a.name == artifact.name) {
        RAISEF(kIndexError, "artifact '$0' already exists in index", a.name);
      }
    }

    index->emplace_back(artifact);
  });
}

void ArtifactIndex::updateStatus(
    const String& artifact_name,
    ArtifactStatus new_status) {
  withIndex(false, [&] (List<ArtifactRef>* index) {
    for (auto& a : *index) {
      if (a.name == artifact_name) {
        statusTransition(&a, new_status);
        return;
      }
    }

    RAISEF(kIndexError, "artifact '$0' not found", artifact_name);
  });
}

void ArtifactIndex::statusTransition(
    ArtifactRef* artifact,
    ArtifactStatus new_status) {
  artifact->status = new_status; // FIXPAUL proper transition
}

void ArtifactIndex::withIndex(
    bool readonly,
    Function<void (List<ArtifactRef>* index)> fn) {
  FileLock lk(index_lockfile_);

  if (!readonly) {
    lk.lock(true);
  }

  auto index = readIndex();
  fn(&index);

  if (!readonly) {
    writeIndex(index);
  }
}

List<ArtifactRef> ArtifactIndex::readIndex() {
  List<ArtifactRef> index;

  if (!(exists_.load() || FileUtil::exists(index_file_))) {
    return index;
  }

  exists_ = true;

  std::unique_lock<std::mutex> lk(cached_mutex_);
  auto mtime = FileUtil::mtime(index_file_);

  if (mtime == cached_mtime_) {
    return cached_;
  }

  auto file = FileUtil::read(index_file_);
  util::BinaryMessageReader reader(file.data(), file.size());
  reader.readUInt8();
  auto num_artifacts = reader.readVarUInt();

  for (int j = 0; j < num_artifacts; ++j) {
    ArtifactRef artifact;

    auto name_len = reader.readVarUInt();
    artifact.name = String((const char*) reader.read(name_len), name_len);
    artifact.status = (ArtifactStatus) ((uint8_t) reader.readVarUInt());

    auto num_attrs = reader.readVarUInt();

    auto num_files = reader.readVarUInt();
    for (int i = 0; i < num_files; ++i) {
      ArtifactFileRef file;
      auto fname_len = reader.readVarUInt();
      file.filename = String((const char*) reader.read(fname_len), fname_len);
      file.checksum = *reader.readUInt64();
      file.size = reader.readVarUInt();
      artifact.files.emplace_back(file);
    }

    index.emplace_back(artifact);
  }

  if (mtime > cached_mtime_) {
    cached_mtime_ = mtime;
    cached_ = index;
  }

  return index;
}

void ArtifactIndex::writeIndex(const List<ArtifactRef>& index) {
  util::BinaryMessageWriter writer;
  writer.appendUInt8(0x01);
  writer.appendVarUInt(index.size());

  for (const auto& a : index) {
    writer.appendVarUInt(a.name.size());
    writer.append(a.name.data(), a.name.size());
    writer.appendVarUInt((uint8_t) a.status);
    writer.appendVarUInt(0); // attributes size
    writer.appendVarUInt(a.files.size());
    for (const auto& f : a.files) {
      writer.appendVarUInt(f.filename.size());
      writer.append(f.filename.data(), f.filename.size());
      writer.appendUInt64(f.checksum);
      writer.appendVarUInt(f.size);
    }
  }

  auto file = File::openFile(
      index_file_ + "~",
      File::O_CREATEOROPEN | File::O_WRITE | File::O_TRUNCATE);

  file.write(writer.data(), writer.size());
  FileUtil::mv(index_file_ + "~", index_file_);

  std::unique_lock<std::mutex> lk(cached_mutex_);
  cached_mtime_ = FileUtil::mtime(index_file_);
  cached_ = index;
}

const String& ArtifactIndex::basePath() const {
  return db_path_;
}

} // namespace eventdb
} // namespace fnord


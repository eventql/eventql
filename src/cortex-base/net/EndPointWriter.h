// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/io/FileRef.h>
#include <memory>
#include <deque>

namespace cortex {

class Buffer;
class BufferRef;
class FileRef;
class EndPoint;

/**
 * Composable EndPoint Writer API.
 *
 * @todo 2 consecutive buffer writes should merge.
 * @todo consider managing its own BufferPool
 */
class CORTEX_API EndPointWriter {
 public:
  EndPointWriter();
  ~EndPointWriter();

  /**
   * Writes given @p data into the chunk queue.
   */
  void write(const BufferRef& data);

  /**
   * Appends given @p data into the chunk queue.
   */
  void write(Buffer&& data);

  /**
   * Appends given chunk represented by given file descriptor and range.
   *
   * @param file file ref to read from.
   */
  void write(FileRef&& file);

  /**
   * Transfers as much data as possible into the given EndPoint @p sink.
   *
   * @retval true all data has been transferred.
   * @retval false data transfer incomplete and data is pending.
   */
  bool flush(EndPoint* sink);

 private:
  class Chunk;
  class BufferChunk;
  class BufferRefChunk;
  class FileChunk;

  std::deque<std::unique_ptr<Chunk>> chunks_;
};

// {{{ Chunk API
class CORTEX_API EndPointWriter::Chunk {
 public:
  virtual ~Chunk() {}

  virtual bool transferTo(EndPoint* sink) = 0;
};

class CORTEX_API EndPointWriter::BufferChunk : public Chunk {
 public:
  explicit BufferChunk(Buffer&& buffer)
      : data_(std::forward<Buffer>(buffer)), offset_(0) {}

  explicit BufferChunk(const BufferRef& buffer)
      : data_(buffer), offset_(0) {}

  explicit BufferChunk(const Buffer& copy)
      : data_(copy), offset_(0) {}

  bool transferTo(EndPoint* sink) override;

 private:
  Buffer data_;
  size_t offset_;
};

class CORTEX_API EndPointWriter::BufferRefChunk : public Chunk {
 public:
  explicit BufferRefChunk(const BufferRef& buffer)
      : data_(buffer), offset_(0) {}

  bool transferTo(EndPoint* sink) override;

 private:
  BufferRef data_;
  size_t offset_;
};

class CORTEX_API EndPointWriter::FileChunk : public Chunk {
 public:
  explicit FileChunk(FileRef&& ref)
      : file_(std::forward<FileRef>(ref)) {}

  ~FileChunk();

  bool transferTo(EndPoint* sink) override;

 private:
  FileRef file_;
};
// }}}

} // namespace cortex

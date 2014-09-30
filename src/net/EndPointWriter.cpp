#include <xzero/net/EndPointWriter.h>
#include <xzero/net/EndPoint.h>
#include <unistd.h>

namespace xzero {

EndPointWriter::EndPointWriter()
    : chunks_() {
}

EndPointWriter::~EndPointWriter() {
}

void EndPointWriter::write(const BufferRef& data) {
  chunks_.emplace_back(std::unique_ptr<Chunk>(new BufferRefChunk(data)));
}

void EndPointWriter::write(Buffer&& chunk) {
  chunks_.emplace_back(std::unique_ptr<Chunk>(
        new BufferChunk(std::forward<Buffer>(chunk))));
}

void EndPointWriter::write(FileRef&& chunk) {
  chunks_.emplace_back(std::unique_ptr<Chunk>(
        new FileChunk(std::forward<FileRef>(chunk))));
}

bool EndPointWriter::flush(EndPoint* sink) {
  while (!chunks_.empty()) {
    if (!chunks_.front()->transferTo(sink))
      return false;

    chunks_.pop_front();
  }

  return true;
}

// {{{ EndPointWriter::BufferChunk
bool EndPointWriter::BufferChunk::transferTo(EndPoint* sink) {
  size_t n = sink->flush(data_.ref(offset_));
  // printf("EndPointWriter.BufferChunk.transferTo(): %zu/%zu bytes written\n",
  //        n, data_.size() - offset_);

  offset_ += n;

  return offset_ == data_.size();
}
// }}}
// {{{ EndPointWriter::BufferRefChunk
bool EndPointWriter::BufferRefChunk::transferTo(EndPoint* sink) {
  size_t n = sink->flush(data_.ref(offset_));
  // printf("BufferRefChunk.transferTo: %zu bytes\n", n);

  offset_ += n;
  return offset_ == data_.size();
}
// }}}
// {{{ EndPointWriter::FileChunk
EndPointWriter::FileChunk::~FileChunk() {
}

bool EndPointWriter::FileChunk::transferTo(EndPoint* sink) {
  const size_t n = sink->flush(file_.handle(), file_.offset(), file_.size());
  // printf("EndPointWriter.FileChunk.transferTo(): %zu/%zu bytes written\n",
  //        n, file_.size());

  file_.setSize(file_.size() - n);
  file_.setOffset(file_.offset() + n);

  return file_.size() == 0;
}
// }}}

} // namespace xzero

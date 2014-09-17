// vim:ts=2:sw=2
#include <xzero/http2.h>
#include <base/Buffer.h>
#include <memory>
#include <vector>
#include <utility>
#include <stdio.h>
#include <arpa/inet.h>

namespace xzero {
namespace framing {

// {{{ free functions
std::string to_string(FrameType type) {
  static const std::string values[] =
      {[0] = "DATA",         [1] = "HEADERS",  [2] = "PRIORITY",
       [3] = "RST_STREAM",   [4] = "SETTINGS", [5] = "PUSH_PROMISE",
       [6] = "PING",         [7] = "GOAWAY",   [8] = "WINDOW_UPDATE",
       [9] = "CONTINUATION", };

  size_t offset = static_cast<size_t>(type);
  if (offset <= 9) {
    return values[offset];
  }

  char buf[32];
  snprintf(buf, sizeof(buf), "UNKNOWN_FRAME_%zu", offset);
  return std::string(buf);
}

std::string to_string(ErrorCode ec) {
  static const std::string values[] =
      {[0x0] = "NO_ERROR",            [0x1] = "PROTOCOL_ERROR",
       [0x2] = "INTERNAL_ERROR",      [0x3] = "FLOW_CONTROL_ERROR",
       [0x4] = "SETTINGS_TIMEOUT",    [0x5] = "STREAM_CLOSED",
       [0x6] = "FRAME_SIZE_ERROR",    [0x7] = "REFUSED_STREAM",
       [0x8] = "CANCEL",              [0x9] = "COMPRESSION_ERROR",
       [0xa] = "CONNECT_ERROR",       [0xb] = "ENHANCE_YOUR_CALM",
       [0xc] = "INADEQUATE_SECURITY", };

  size_t offset = static_cast<size_t>(ec);
  if (offset <= 0xc) {
    return values[offset];
  }

  char buf[32];
  size_t len = snprintf(buf, sizeof(buf), "UNKNOWN_ERROR_%02zx", offset);
  return std::string(buf, len);
}

std::string to_string(uint8_t flags, FrameType type) {
  static const std::unordered_map<unsigned, std::map<unsigned, std::string>>
  mapping = {{(unsigned)FrameType::DATA,
              {{DataFrame::END_STREAM, "END_STREAM"},    // 0x01
               {DataFrame::END_SEGMENT, "END_SEGMENT"},  // 0x02
               {DataFrame::PADDED, "PADDED"}             // 0x08
              }},
             {(unsigned)FrameType::HEADERS,
              {{HeadersFrame::END_STREAM, "END_STREAM"},    // 0x01
               {HeadersFrame::END_SEGMENT, "END_SEGMENT"},  // 0x02
               {HeadersFrame::END_HEADERS, "END_HEADERS"},  // 0x04
               {HeadersFrame::PADDED, "PADDED"},            // 0x08
               {HeadersFrame::PRIORITY, "PRIORITY"}         // 0x20
              }},
             {(unsigned)FrameType::PRIORITY,
              {/* no flags */
              }},
             {(unsigned)FrameType::RST_STREAM,
              {/* no flags */
              }},
             {(unsigned)FrameType::SETTINGS,
              {{SettingsFrame::ACK, "ACK"}  // 0x01
              }},
             {(unsigned)FrameType::PUSH_PROMISE,
              {{PushPromiseFrame::END_HEADERS, "END_HEADERS"},  // 0x04
               {PushPromiseFrame::PADDED, "PADDED"}             // 0x08
              }},
             {(unsigned)FrameType::PING,
              {{PingFrame::ACK, "ACK"},  // 0x01
              }},
             {(unsigned)FrameType::GOAWAY,
              {/* no flags */
              }},
             {(unsigned)FrameType::WINDOW_UPDATE,
              {/* no flags */
              }},
             {(unsigned)FrameType::CONTINUATION,
              {{HeadersFrame::END_HEADERS, "END_HEADERS"},  // 0x04
              }}};

  const auto i = mapping.find(static_cast<unsigned>(type));
  if (i == mapping.end()) {
    fprintf(stderr, "BUG! Unknown frame type: %d\n", type);
    abort();
  }

  const auto& flagsMap = i->second;
  std::string out;

  for (unsigned i = 0; i < 8; ++i) {
    auto k = flagsMap.find(1 << i);
    if (flags & (1 << i)) {
      if (k != flagsMap.end()) {
        if (!out.empty()) out += ",";
        out += k->second;
      } else {
        if (!out.empty()) out += ",";
        char buf[8];
        snprintf(buf, sizeof(buf), "0x%02x", (1 << i));
        out += buf;
      }
    }
  }

  return out;
}
// }}}
// {{{ Frame
void Frame::dumpHeader() const {
  printf(" * %s: flags=[%s], stream_id=%u, length=%zu\n",
         to_string(type()).c_str(), to_string(flags(), type()).c_str(),
         streamID(), payloadLength());
}

void Frame::setHeader(size_t payloadLength, FrameType type, unsigned flags,
                      unsigned streamID) {
  reserved1_ = 0;
  length_ = htons(payloadLength);
  type_ = (unsigned)type;
  flags_ = flags;
  reserved2_ = 0;
  streamID_ = htonl(streamID);
}

void Frame::encodeHeader(Buffer* out, size_t payloadLength, FrameType type,
                         unsigned flags, unsigned streamID) {
  Frame frame;

  frame.setHeader(payloadLength, type, flags, streamID);

  out->push_back(&frame, sizeof(frame));
}
// }}}
// {{{ DataFrame
void DataFrame::dump() const {
  dumpHeader();
  // data().dump("DATA-frame data");
  // padding().dump("DATA-frame padding");
}

void DataFrame::encode(Buffer* out, bool endStream, bool endSegment,
                       unsigned streamID, const BufferRef& data) {
  unsigned flags = 0;

  if (endStream) flags |= END_STREAM;

  if (endSegment) flags |= END_SEGMENT;

  uint32_t pre = 0;

  encodeHeader(out, sizeof(pre) + data.size(), FrameType::DATA, flags,
               streamID);
  out->push_back(&pre, sizeof(pre));
  out->push_back(data);
}

void DataFrame::encode(Buffer* out, bool endStream, bool endSegment,
                       unsigned streamID, const BufferRef& data,
                       const BufferRef& padding) {
  assert(padding.size() < 0xFF);

  unsigned flags = PADDED;

  if (endStream) flags |= END_STREAM;

  if (endSegment) flags |= END_SEGMENT;

  char pre[4];
  pre[0] = padding.size();
  pre[1] = '\0';
  pre[2] = '\0';
  pre[3] = '\0';

  encodeHeader(out, sizeof(pre) + data.size() + padding.size(), FrameType::DATA,
               flags, streamID);
  out->push_back(&pre, sizeof(pre));
  out->push_back(data);
  out->push_back(padding);
}
// }}}
// {{{ HeadersFrame
void HeadersFrame::dump() const {
  dumpHeader();
  // TODO
}
// }}}
// {{{ PriorityFrame
void PriorityFrame::dump() const {
  dumpHeader();

  printf("    ; exclusive=%s, dependant_stream_id=%u, weight=%u\n",
         isExclusive() ? "true" : "false", dependantStreamID(), weight());
}

void PriorityFrame::encode(Buffer* out, bool exclusive,
                           unsigned dependantStreamID, uint8_t weight,
                           unsigned streamID) {
  PriorityFrame frame;

  frame.setHeader(sizeof(frame) - sizeof(Frame), FrameType::PRIORITY, 0,
                  streamID);
  frame.isExclusive_ = exclusive;
  frame.dependantStreamID_ = htonl(dependantStreamID);
  frame.weight_ = weight;
  frame.unused_ = 0;

  out->push_back(&frame, sizeof(frame));
}
// }}}
// {{{ ResetStreamFrame
void ResetStreamFrame::dump() const {
  dumpHeader();
  printf("    ; errorCode: %s\n", to_string(errorCode()).c_str());
}

void ResetStreamFrame::encode(Buffer* out, ErrorCode ec, unsigned streamID) {
  ResetStreamFrame frame;

  frame.setHeader(sizeof(frame) - sizeof(Frame), FrameType::RST_STREAM, 0,
                  streamID);
  frame.errorCode_ = htonl(static_cast<unsigned>(ec));

  out->push_back(&frame, sizeof(frame));
}
// }}}
// {{{ SettingsFrame
void SettingsFrame::dump() const {
  dumpHeader();

  size_t e = parameterCount();
  for (size_t i = 0; i != e; ++i) {
    Parameter param = parameter(i);
    printf("    ; %s (%02x) -> 0x%04x\n", param.name(), param.type(),
           param.value());
  }
}

SettingsFrame::Parameter SettingsFrame::Parameter::encode(Type type,
                                                          uint32_t value) {
  Parameter param;

  param.type_ = htons(type);
  param.unused_ = 0;
  param.value_ = htonl(value);

  return param;
}

void SettingsFrame::encode(
    Buffer* out,
    const std::vector<std::pair<Parameter::Type, uint32_t>>& params) {
  encodeHeader(out, params.size() * sizeof(Parameter), FrameType::SETTINGS, 0,
               0);

  for (const auto& param : params) {
    Parameter enc = Parameter::encode(param.first, param.second);
    out->push_back((char*)&enc, sizeof(enc));
  }
}

void SettingsFrame::encodeAck(Buffer* out) {
  encodeHeader(out, 0, FrameType::SETTINGS, Flags::ACK, 0);
}
// }}}
// {{{ PushPromiseFrame
void PushPromiseFrame::dump() const {
  dumpHeader();
  // TODO
}
// }}}
// {{{ PingFrame
void PingFrame::dump() const {
  dumpHeader();
  printf("    ; opaqueData = 0x%08lx\n", data());
}

void PingFrame::encode(Buffer* out, uint64_t data) {
  encodeHeader(out, sizeof(uint64_t), FrameType::PING, 0, 0);

  char buf[8];
  buf[0] = (data >> 0) & 0xFF;
  buf[1] = (data >> 8) & 0xFF;
  buf[2] = (data >> 16) & 0xFF;
  buf[3] = (data >> 24) & 0xFF;
  buf[4] = (data >> 32) & 0xFF;
  buf[5] = (data >> 40) & 0xFF;
  buf[6] = (data >> 48) & 0xFF;
  buf[7] = (data >> 56) & 0xFF;

  out->push_back(buf, sizeof(buf));
}

void PingFrame::encodeAck(Buffer* out, uint64_t data) {
  encodeHeader(out, sizeof(uint64_t), FrameType::PING, Flags::ACK, 0);

  char buf[8];
  buf[0] = (data >> 0) & 0xFF;
  buf[1] = (data >> 8) & 0xFF;
  buf[2] = (data >> 16) & 0xFF;
  buf[3] = (data >> 24) & 0xFF;
  buf[4] = (data >> 32) & 0xFF;
  buf[5] = (data >> 40) & 0xFF;
  buf[6] = (data >> 48) & 0xFF;
  buf[7] = (data >> 56) & 0xFF;

  out->push_back(buf, sizeof(buf));
}
// }}}
// {{{ GoAwayFrame
/*
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |R|                  Last-Stream-ID (31)                        |
 * +-+-------------------------------------------------------------+
 * |                      Error Code (32)                          |
 * +---------------------------------------------------------------+
 * |                  Additional Debug Data (*)                    |
 * +---------------------------------------------------------------+
 */
void GoAwayFrame::dump() const {
  dumpHeader();
  printf("    ; lastStreamID=%u, errorCode=%s\n", lastStreamID(),
         to_string(errorCode()).c_str());

  BufferRef debug = debugData();

  if (!debug.empty()) {
    debug.dump("    ; AdditionalDebugData");
  }
}

void GoAwayFrame::encode(Buffer* out, unsigned lastStreamID, ErrorCode ec) {
  GoAwayFrame frame;

  frame.setHeader(sizeof(frame) - sizeof(Frame), FrameType::GOAWAY, 0, 0);
  frame.reserved_ = 0;
  frame.lastStreamID_ = htonl(lastStreamID);
  frame.errorCode_ = htonl(static_cast<unsigned>(ec));

  out->push_back(&frame, sizeof(frame));
}

void GoAwayFrame::encode(Buffer* out, unsigned lastStreamID, ErrorCode ec,
                         const BufferRef& debugData) {
  GoAwayFrame frame;

  frame.setHeader(sizeof(frame) - sizeof(Frame) + debugData.size(),
                  FrameType::GOAWAY, 0, 0);
  frame.reserved_ = 0;
  frame.lastStreamID_ = htonl(lastStreamID);
  frame.errorCode_ = htonl(static_cast<unsigned>(ec));

  out->push_back(&frame, sizeof(frame) + debugData.size());
  out->push_back(debugData);
}
// }}}
// {{{ WindowUpdateFrame
void WindowUpdateFrame::dump() const {
  dumpHeader();
  printf("    ; windowSizeIncrement=%u\n", windowSizeIncrement());
}

void WindowUpdateFrame::encode(Buffer* out, unsigned windowSizeIncrement) {
  assert(windowSizeIncrement <= 0x7FFF);

  WindowUpdateFrame frame;
  frame.setHeader(sizeof(frame) - sizeof(Frame), FrameType::WINDOW_UPDATE, 0,
                  0);
  frame.reserved_ = 0;
  frame.windowSizeIncrement_ = htonl(windowSizeIncrement);

  out->push_back(&frame, sizeof(frame));
}
// }}}
// {{{ ContinuationFrame
void ContinuationFrame::dump() const {
  dumpHeader();
  // TODO
}
// }}}

}  // namespace framing
}  // namespace xzero

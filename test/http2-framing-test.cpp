// vim:ts=2:sw=2
#include <gtest/gtest.h>
#include <xzero/http2.h>

using namespace base;
using namespace xzero;
using namespace xzero::framing;

void dumpFrames(const BufferRef& buf)  // {{{
{
  size_t i = 0;

  while (i + sizeof(framing::Frame) <= buf.size() &&
         i + ((framing::Frame*)buf.data() + i)->payloadLength() <= buf.size()) {

    const framing::Frame* frame = (framing::Frame*)(buf.data() + i);

    switch (frame->type()) {
      case framing::FrameType::DATA:
        static_cast<const framing::DataFrame*>(frame)->dump();
        break;
      case framing::FrameType::HEADERS:
        static_cast<const framing::HeadersFrame*>(frame)->dump();
        break;
      case framing::FrameType::PRIORITY:
        static_cast<const framing::PriorityFrame*>(frame)->dump();
        break;
      case framing::FrameType::RST_STREAM:
        static_cast<const framing::ResetStreamFrame*>(frame)->dump();
        break;
      case framing::FrameType::SETTINGS:
        static_cast<const framing::SettingsFrame*>(frame)->dump();
        break;
      case framing::FrameType::PUSH_PROMISE:
        static_cast<const framing::PushPromiseFrame*>(frame)->dump();
        break;
      case framing::FrameType::PING:
        static_cast<const framing::PingFrame*>(frame)->dump();
        break;
      case framing::FrameType::GOAWAY:
        static_cast<const framing::GoAwayFrame*>(frame)->dump();
        break;
      case framing::FrameType::WINDOW_UPDATE:
        static_cast<const framing::WindowUpdateFrame*>(frame)->dump();
        break;
      case framing::FrameType::CONTINUATION:
        static_cast<const framing::ContinuationFrame*>(frame)->dump();
        break;
      default:
        printf("Unsupported frame type: 0x%02x\n", frame->type());
        break;
    }

    i += sizeof(*frame) + frame->payloadLength();
  }
}  // }}}

TEST(Http2_SettingsFrame, encode) {
  static const std::vector<
      std::pair<framing::SettingsFrame::Parameter::Type, uint32_t>> settings =
      {{framing::SettingsFrame::Parameter::HeaderTableSize, 100},
       {framing::SettingsFrame::Parameter::EnablePush, 0x123456},
       {framing::SettingsFrame::Parameter::MaxConcurrentStreams, 100},
       {framing::SettingsFrame::Parameter::InitialWindowSize, 65535}, };

  Buffer buf;
  framing::SettingsFrame::encode(&buf, settings);
  //  buf.dump("blob");
  //  dumpFrames(buf);
}

TEST(Http2_SettingsFrame, encodeAck) {
  Buffer buf;
  framing::SettingsFrame::encodeAck(&buf);
}

TEST(Http2_DataFrame, encode) {
  Buffer buf;
  framing::DataFrame::encode(&buf, true, true, 42, BufferRef("56789"));
}

TEST(Http2_DataFrame, encodePadded) {
  Buffer buf;
  framing::DataFrame::encode(&buf, true, true, 42, BufferRef("Hello"),
                             BufferRef(" World!"));
}

TEST(Http2_PingFrame, encode) {
  Buffer buf;
  framing::PingFrame::encode(&buf, 0x0123456789abcdef);
}

TEST(Http2_PingFrame, encodeAck) {
  Buffer buf;
  framing::PingFrame::encodeAck(&buf, 0x0123456789abcdef);
}

TEST(Http2_PriorityFrame, encode) {
  Buffer buf;
  framing::PriorityFrame::encode(&buf, false, 26, 4, 91);
}

TEST(Http2_ResetStreamFrame, encode) {
  Buffer buf;
  framing::ResetStreamFrame::encode(&buf, framing::ErrorCode::StreamClosed, 42);
}

TEST(Http2_GoAwayFrame, encode) {
  Buffer buf;
  framing::GoAwayFrame::encode(&buf, 42, framing::ErrorCode::ProtocolError);
}

TEST(Http2_WindowUpdateFrame, encode) {
  Buffer buf;
  framing::WindowUpdateFrame::encode(&buf, 0x7FFF);
  // buf.dump("blob");
  // dumpFrames(buf);
}

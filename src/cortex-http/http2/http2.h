// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

// vim:ts=2:sw=2
#pragma once

#include <cortex-http/Api.h>
#include <base/Buffer.h>

#include <memory>
#include <vector>
#include <utility>
#include <stdint.h>
#include <unordered_map>
#include <map>
#include <arpa/inet.h>

namespace cortex {

using namespace base;

namespace framing {

// {{{ enum types
enum class FrameType {
  DATA = 0,
  HEADERS = 1,
  PRIORITY = 2,
  RST_STREAM = 3,
  SETTINGS = 4,
  PUSH_PROMISE = 5,
  PING = 6,
  GOAWAY = 7,
  WINDOW_UPDATE = 8,
  CONTINUATION = 9,
};

enum class ErrorCode {
  /**
   * The associated condition is not as a result of an
   * error. For example, a GOAWAY might include this code to indicate
   * graceful shutdown of a connection.
   */
  NoError = 0,

  /**
   * The endpoint detected an unspecific protocol error.
   * This error is for use when a more specific error code is
   * not available.
   */
  ProtocolError = 1,

  /**
   * The endpoint encountered an unexpected internal error.
   */
  InternalError = 2,

  /**
   * The endpoint detected that its peer violated the flow control protocol.
   */
  FlowControlError = 3,

  /**
   * The endpoint sent a SETTINGS frame, but did
   * not receive a response in a timely manner.
   * See Settings Synchronization (Section 6.5.3).
   */
  SettingsTimeout = 4,

  /**
   * The endpoint received a frame after a stream was half closed.
   */
  StreamClosed = 5,

  /**
   * The endpoint received a frame that was larger
   * than the maximum size that it supports.
   */
  FrameSizeError = 6,

  /**
   * The endpoint refuses the stream prior to
   * performing any application processing, see Section 8.1.4 for
   * details.
   */
  RefusedStream = 7,

  /**
   * Used by the endpoint to indicate that the stream is no longer needed.
   */
  Cancel = 8,

  /**
   * The endpoint is unable to maintain the compression context for the
   * connection.
   */
  CompressionError = 9,

  /**
   * The connection established in response to a
   * CONNECT request (Section 8.3) was reset or abnormally closed.
   */
  ConnectError = 10,

  /**
   * The endpoint detected that its peer is
   * exhibiting a behavior over a given amount of time that has caused
   * it to refuse to process further frames.
   */
  EnhanceYourCalm = 11,
};
// }}}
// {{{ free functions
CORTEX_HTTP_API std::string to_string(FrameType type);
CORTEX_HTTP_API std::string to_string(ErrorCode ec);
CORTEX_HTTP_API std::string to_string(uint8_t flags, FrameType type);
// }}}
struct CORTEX_HTTP_API BASE_PACKED Frame {  // {{{
                                 /*
*  0                   1                   2                   3
*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* | R |     Length (14)           |   Type (8)    |   Flags (8)   |
* +-+-+-----------+---------------+-------------------------------+
* |R|                 Stream Identifier (31)                      |
* +=+=============================================================+
*/
 public:
  FrameType type() const { return static_cast<FrameType>(type_); }
  uint8_t flags() const { return flags_; }
  unsigned streamID() const { return ntohl(streamID_); }

  uint8_t* payloadData() { return ((uint8_t*)this) + sizeof(*this); }
  const uint8_t* payloadData() const {
    return ((uint8_t*)this) + sizeof(*this);
  }
  size_t payloadLength() const { return ntohs(length_); }

  BufferRef payload() const {
    return BufferRef(((const char*)this) + sizeof(*this), payloadLength());
  };
  BufferRef payload() {
    return BufferRef(((char*)this) + sizeof(*this), payloadLength());
  };

  void dumpHeader() const;
  void dump() const;

  static void encodeHeader(Buffer* out, size_t payloadLength, FrameType type,
                           unsigned flags, unsigned streamID);

 protected:
  void setHeader(size_t payloadLength, FrameType type, unsigned flags,
                 unsigned streamID);

 protected:
  unsigned reserved1_ : 2;
  unsigned length_ : 14;
  unsigned type_ : 8;
  unsigned flags_ : 8;
  unsigned reserved2_ : 1;
  unsigned streamID_ : 31;
};
// }}}
struct CORTEX_HTTP_API BASE_PACKED HeadersFrame : public Frame {  // {{{
                                                       /*
*  0                   1                   2                   3
*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* |Pad Length? (8)|
* +-+-------------+-----------------------------------------------+
* |E|                 Stream Dependency? (31)                     |
* +-+-------------+-----------------------------------------------+
* |  Weight? (8)  |
* +-+-------------+-----------------------------------------------+
* |                   Header Block Fragment (*)                 ...
* +---------------------------------------------------------------+
* |                           Padding (*)                       ...
* +---------------------------------------------------------------+
*/
 public:
  enum Flags {
    END_STREAM = 0x01,
    END_SEGMENT = 0x02,
    END_HEADERS = 0x04,
    PADDED = 0x08,
    PRIORITY = 0x20,
  };

  // flag tests
  bool isEndStream() const { return flags() & END_STREAM; }
  bool isEndSegment() const { return flags() & END_STREAM; }
  bool isEndHeaders() const { return flags() & END_HEADERS; }
  bool isPadded() const { return flags() & PADDED; }
  bool isPriorized() const { return flags() & PRIORITY; }

  // priority extensions
  bool isExclusiveStreamDependency() const {
    return isExclusiveStreamDependency_;
  }
  size_t streamDependencyID() const { return streamDependencyID_; }
  uint8_t weight() const { return weight_; }

  // data
  BufferRef headerBlockFragment() const {
    return payload().ref(12, payloadLength() - 12 - padLength_);
  }
  BufferRef padding() const {
    return payload().ref(payloadLength() - padLength_);
  }

  void dump() const;

  // static void encode(Buffer* out,
  //    bool exclusive, unsigned depID, uint8_t weight, const HeaderTable&
  // headers);

 protected:
  uint8_t padLength_;
  unsigned unused1_ : 24;
  unsigned isExclusiveStreamDependency_ : 1;
  unsigned streamDependencyID_ : 31;
  uint8_t weight_;
  unsigned unused2_ : 24;
};
// }}}
struct CORTEX_HTTP_API BASE_PACKED ContinuationFrame : public Frame {  // {{{
                                                            /*
*  0                   1                   2                   3
*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* |                   Header Block Fragment (*)                 ...
* +---------------------------------------------------------------+
*/
 public:
  enum Flags { END_HEADERS = 0x04, };

  // flag tests
  bool isEndHeaders() const { return flags() & END_HEADERS; }

  // data
  BufferRef headerBlockFragment() const {
    return payload().ref(12, payloadLength() - 12 - padLength_);
  }

  void dump() const;

 protected:
  uint8_t padLength_;
  unsigned unused1_ : 24;
  unsigned isExclusiveStreamDependency_ : 1;
  unsigned streamDependencyID_ : 31;
  uint8_t weight_;
  unsigned unused2_ : 24;
};
// }}}
struct CORTEX_HTTP_API BASE_PACKED PriorityFrame : public Frame {  // {{{
                                                        /*
*  0                   1                   2                   3
*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* |E|                  Stream Dependency (31)                     |
* +-+-------------+-----------------------------------------------+
* |   Weight (8)  |
* +-+-------------+
*/
 public:
  bool isExclusive() const { return isExclusive_; }
  unsigned dependantStreamID() const { return ntohl(dependantStreamID_); }
  uint8_t weight() const { return weight_; }

  void dump() const;

  static void encode(Buffer* out, bool exclusive, unsigned dependantStreamID,
                     uint8_t weight, unsigned streamID);

 private:
  unsigned isExclusive_ : 1;
  unsigned dependantStreamID_ : 31;
  unsigned weight_ : 8;
  unsigned unused_ : 24;
};
// }}}
struct CORTEX_HTTP_API BASE_PACKED DataFrame : public Frame {  // {{{
                                                    /*
*  0                   1                   2                   3
*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* |Pad Length? (8)|
* +---------------+-----------------------------------------------+
* |                            Data (*)                         ...
* +---------------------------------------------------------------+
* |                           Padding (*)                       ...
* +---------------------------------------------------------------+
*/
 public:
  enum Flags { END_STREAM = 0x01, END_SEGMENT = 0x02, PADDED = 0x08, };

  uint8_t padLength() const { return padLength_; }
  BufferRef data() const {
    return payload().ref(4, payloadLength() - 4 - padLength());
  }
  BufferRef padding() const {
    return payload().ref(payloadLength() - padLength());
  }

  void dump() const;

  static void encode(Buffer* out, bool endStream, bool endSegment,
                     unsigned streamID, const BufferRef& data);
  static void encode(Buffer* out, bool endStream, bool endSegment,
                     unsigned streamID, const BufferRef& data,
                     const BufferRef& padding);

 private:
  uint8_t padLength_;
};
// }}}
struct CORTEX_HTTP_API BASE_PACKED PingFrame : public Frame {  // {{{
 public:
  enum Flags { ACK = 0x01, };

  bool isAcknowledgement() const { return flags() & ACK; }
  uint64_t data() const { return opaqueData_; }

  void dump() const;

  static void encode(Buffer* out, uint64_t data);
  static void encodeAck(Buffer* out, uint64_t data);

 private:
  uint64_t opaqueData_;
};
// }}}
struct CORTEX_HTTP_API BASE_PACKED GoAwayFrame : public Frame {  // {{{
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
 public:
  uint32_t lastStreamID() const {
    return static_cast<uint32_t>(ntohl(lastStreamID_));
  }
  ErrorCode errorCode() const {
    return static_cast<ErrorCode>(ntohl(errorCode_));
  }
  BufferRef debugData() const { return payload().ref(8); }

  void dump() const;

  static void encode(Buffer* out, unsigned lastStreamID, ErrorCode ec);

  static void encode(Buffer* out, unsigned lastStreamID, ErrorCode ec,
                     const BufferRef& debugData);

 private:
  unsigned reserved_ : 1;
  unsigned lastStreamID_ : 31;
  unsigned errorCode_ : 32;
};
// }}}
struct CORTEX_HTTP_API BASE_PACKED ResetStreamFrame : public Frame {  // {{{
 public:
  ErrorCode errorCode() const {
    return static_cast<ErrorCode>(ntohl(errorCode_));
  }

  void dump() const;

  static void encode(Buffer* out, ErrorCode ec, unsigned streamID);

 private:
  uint32_t errorCode_;
};
// }}}
struct CORTEX_HTTP_API BASE_PACKED SettingsFrame : public Frame {  // {{{
 public:
  enum Flags { ACK = 0x01 };

  struct BASE_PACKED Parameter {  // {{{
                                /*
*  0                   1                   2                   3
*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* |       Identifier (16)         |
* +-------------------------------+-------------------------------+
* |                        Value (32)                             |
* +---------------------------------------------------------------+
*/
    enum Type {
      HeaderTableSize = 1,
      EnablePush = 2,
      MaxConcurrentStreams = 3,
      InitialWindowSize = 4
    };

    Type type() const { return static_cast<Type>(ntohs(type_)); }

    const char* name() const {
      switch (type()) {
        case HeaderTableSize:
          return "HeaderTableSize";
        case EnablePush:
          return "EnablePush";
        case MaxConcurrentStreams:
          return "MaxConcurrentStreams";
        case InitialWindowSize:
          return "InitialWindowSize";
        default: {
          return "UNKNOWN_PARAM";
          // abort();
        }
      }
    }

    uint32_t value() const { return ntohl(static_cast<uint32_t>(value_)); }

    static Parameter encode(Type type, uint32_t value);

   private:
    unsigned type_ : 16;
    unsigned unused_ : 16;
    unsigned value_ : 32;
  };                 // }}}
  struct iterator {  // {{{
   public:
    iterator() = default;
    iterator(const iterator& other) = default;
    iterator(const SettingsFrame* frame, size_t offset)
        : frame_(frame), offset_(offset) {}

    bool operator==(const iterator& other) const {
      return offset_ == other.offset_;
    }
    bool operator!=(const iterator& other) const {
      return offset_ != other.offset_;
    }

    iterator& operator++() {
      if (offset_ < frame_->parameterCount()) {
        ++offset_;
      }
      return *this;
    }

    Parameter operator*() const { return frame_->parameter(offset_); }

   private:
    const SettingsFrame* frame_;
    size_t offset_;
  };  // }}}

  // flags
  bool isAcknowledgement() const { return flags() & ACK; }

  size_t parameterCount() const { return payloadLength() / 8; }

  Parameter parameter(size_t offset) const {
    return *reinterpret_cast<const Parameter*>(payloadData() + 8 * offset);
  }

  void dump() const;

  iterator begin() const { return iterator(this, 0); }
  iterator end() const { return iterator(this, parameterCount()); }

  static void encode(
      Buffer* out,
      const std::vector<std::pair<Parameter::Type, uint32_t>>& params);
  static void encodeAck(Buffer* out);
};                                                         // }}}
struct CORTEX_HTTP_API BASE_PACKED PushPromiseFrame : public Frame {  // {{{
                                                           /*
*  0                   1                   2                   3
*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* |Pad Length? (8)|
* +-+-------------+-----------------------------------------------+
* |R|                  Promised Stream ID (31)                    |
* +-+-----------------------------+-------------------------------+
* |                   Header Block Fragment (*)                 ...
* +---------------------------------------------------------------+
* |                           Padding (*)                       ...
* +---------------------------------------------------------------+
*/
 public:
  enum Flags { END_HEADERS = 0x04, PADDED = 0x08, };

  size_t promisedStreamID() const { return promisedStreamID_; }
  BufferRef headerBlockFragment() const {
    return payload().ref(8, payloadLength() - 8 - padLength_);
  }

  void dump() const;

 private:
  unsigned padLength_ : 8;
  unsigned unused_ : 24;
  unsigned reserved_ : 1;
  unsigned promisedStreamID_ : 31;
};
// }}}
struct CORTEX_HTTP_API BASE_PACKED WindowUpdateFrame : public Frame {  // {{{
 public:
  unsigned windowSizeIncrement() const {
    return static_cast<size_t>(ntohl(windowSizeIncrement_));
  }

  void dump() const;

  static void encode(Buffer* out, unsigned windowSizeIncrement);

 private:
  unsigned reserved_ : 1;
  unsigned windowSizeIncrement_ : 31;
};  // }}}

}  // namespace framing
}  // namespace cortex

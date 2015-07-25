// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/mock/Transport.h>
#include <cortex-http/mock/Input.h>
#include <cortex-http/HttpHandler.h>
#include <cortex-http/HttpResponseInfo.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpInput.h>
#include <cortex-http/HttpChannel.h>
#include <cortex-http/BadMessage.h>
#include <cortex-base/executor/Executor.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/io/FileRef.h>
#include <stdexcept>
#include <system_error>

namespace cortex {
namespace http {
namespace mock {

Transport::Transport(Executor* executor, const HttpHandler& handler)
    : Transport(executor, handler, 32, 64, nullptr, nullptr) {
}

Transport::Transport(Executor* executor,
                             const HttpHandler& handler,
                             size_t maxRequestUriLength,
                             size_t maxRequestBodyLength,
                             HttpDateGenerator* dateGenerator,
                             HttpOutputCompressor* outputCompressor)
    : executor_(executor),
      handler_(handler),
      maxRequestUriLength_(maxRequestUriLength),
      maxRequestBodyLength_(maxRequestBodyLength),
      dateGenerator_(dateGenerator),
      outputCompressor_(outputCompressor),
      isAborted_(false),
      isCompleted_(false),
      channel_(),
      responseInfo_(),
      responseBody_() {
}

Transport::~Transport() {
}

void Transport::run(HttpVersion version, const std::string& method,
                        const std::string& entity,
                        const HeaderFieldList& headers,
                        const std::string& body) {
  isCompleted_ = false;
  isAborted_ = false;

  std::unique_ptr<Input> input(new Input());
  channel_.reset(new HttpChannel(this, handler_, std::move(input),
                                 maxRequestUriLength_, maxRequestBodyLength_,
                                 dateGenerator_,
                                 outputCompressor_));


  try {
    channel_->onMessageBegin(BufferRef(method), BufferRef(entity), version);

    for (const HeaderField& header: headers) {
      channel_->onMessageHeader(BufferRef(header.name()),
                                BufferRef(header.value()));
    }

    channel_->onMessageHeaderEnd();
    channel_->onMessageContent(BufferRef(body.data(), body.size()));
    channel_->onMessageEnd();
  } catch (const BadMessage& e) {
    channel_->response()->sendError(e.httpCode(), e.what());
  } catch (const RuntimeError& e) {
    channel_->response()->sendError(HttpStatus::InternalServerError, e.what());
  }
}

void Transport::abort() {
  isAborted_ = true;
}

void Transport::completed() {
  isCompleted_ = true;

  responseInfo_.setTrailers(channel_->response()->trailers());
}

void Transport::send(HttpResponseInfo&& responseInfo,
                         const BufferRef& chunk,
                         CompletionHandler onComplete) {
  responseInfo_ = std::move(responseInfo);
  responseBody_ += chunk;

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void Transport::send(HttpResponseInfo&& responseInfo,
                         Buffer&& chunk,
                         CompletionHandler onComplete) {
  responseInfo_ = std::move(responseInfo);
  responseBody_ += chunk;

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void Transport::send(HttpResponseInfo&& responseInfo,
                         FileRef&& chunk,
                         CompletionHandler onComplete) {
  responseInfo_ = std::move(responseInfo);

  chunk.fill(&responseBody_);

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void Transport::send(const BufferRef& chunk, CompletionHandler onComplete) {
  responseBody_ += chunk;

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void Transport::send(Buffer&& chunk, CompletionHandler onComplete) {
  responseBody_ += chunk;

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void Transport::send(FileRef&& chunk, CompletionHandler onComplete) {
  chunk.fill(&responseBody_);

  if (onComplete) {
    executor()->execute([onComplete]() {
      onComplete(true);
    });
  }
}

} // namespace mock
} // namespace http
} // namespace cortex
